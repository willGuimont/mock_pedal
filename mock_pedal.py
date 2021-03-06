import argparse
import queue
import random
import threading
import time
import typing
from copy import copy
from dataclasses import dataclass
from enum import Enum

import serial
from pynput import keyboard


@dataclass
class Message:
    msg_type: Enum
    data: typing.Optional[typing.Any] = None


class WriterMessageType(Enum):
    EMERGENCY_STOP = -1
    STOP = 0
    WRITE_KEYS = 1


class KeyEvent(Enum):
    RELEASE = 0
    PRESS = 1


@dataclass
class WriterMessageData:
    keys: [(KeyEvent, str)]


class ListenerMessageType(Enum):
    EMERGENCY_STOP = -1
    STOP = 0
    DISABLE = 1
    ENABLE = 2


class PedalMessageType(Enum):
    STOP = 0


def writer_thread(writer_queue: queue.Queue):
    def key_to_text_size(k):
        if hasattr(k, 'char') or k == keyboard.Key.space:
            return 1
        elif k == keyboard.Key.backspace:
            return -1
        else:
            return 0

    controller = keyboard.Controller()
    while True:
        msg: Message = writer_queue.get()
        if msg.msg_type == WriterMessageType.WRITE_KEYS:
            num_chars = sum([key_to_text_size(k) for e, k in msg.data])
            for _ in range(num_chars):
                controller.press(keyboard.Key.backspace)
                controller.release(keyboard.Key.backspace)
                time.sleep(0.005)

            for e, k in msg.data:
                shift_pressed = False
                if hasattr(k, 'char') and k.char.isalpha() and random.randint(0, 1) == 0:
                    controller.press(keyboard.Key.shift)
                    shift_pressed = True

                if e == KeyEvent.PRESS:
                    controller.press(k)
                    controller.release(k)

                if shift_pressed:
                    controller.release(keyboard.Key.shift)

                time.sleep(0.005)
        elif msg.msg_type == WriterMessageType.STOP:
            break
        elif msg.msg_type == WriterMessageType.EMERGENCY_STOP:
            break
        else:
            raise ValueError(f'Invalid message {msg}')
        time.sleep(0.01)


def listener_thread(listener_queue: queue.Queue, writer_queue: queue.Queue):
    mutex = threading.Lock()
    enabled = False
    key_buffer = []

    def on_press(key):
        with mutex:
            if enabled:
                key_buffer.append((KeyEvent.PRESS, key))

    def on_release(key):
        with mutex:
            if enabled:
                key_buffer.append((KeyEvent.RELEASE, key))

    listener = keyboard.Listener(on_press=on_press, on_release=on_release)
    listener.start()
    while True:
        msg: Message = listener_queue.get()
        if msg.msg_type == ListenerMessageType.ENABLE:
            with mutex:
                enabled = True
                key_buffer.clear()
        elif msg.msg_type == ListenerMessageType.DISABLE:
            with mutex:
                writer_queue.put(Message(WriterMessageType.WRITE_KEYS, copy(key_buffer)))
                enabled = False
                key_buffer.clear()
        elif msg.msg_type == ListenerMessageType.STOP:
            break
        elif msg.msg_type == ListenerMessageType.EMERGENCY_STOP:
            writer_queue.put(Message(WriterMessageType.EMERGENCY_STOP))
            break
        else:
            raise ValueError(f'Invalid message {msg}')
        time.sleep(0.01)
    listener.stop()


def pedal_thread(arduino: serial.Serial, pedal_queue: queue.Queue, listener_queue: queue.Queue):
    while True:
        try:
            data = arduino.readline()
        except serial.SerialException:
            listener_queue.put(Message(ListenerMessageType.EMERGENCY_STOP))
            print('Pedal was disconnected.\nExiting.')
            break

        if data == b'+\n':
            listener_queue.put(Message(ListenerMessageType.ENABLE))
        elif data == b'-\n':
            listener_queue.put(Message(ListenerMessageType.DISABLE))

        if not pedal_queue.empty():
            msg: Message = pedal_queue.get()
            if msg.msg_type == PedalMessageType.STOP:
                break
            else:
                raise ValueError(f'Invalid message {msg}')
        time.sleep(0.01)


def main(port):
    try:
        arduino = serial.Serial(port=port, baudrate=9600, timeout=0.1)
    except serial.SerialException:
        print(f'Could not open port {port}.\nExiting.')
        return 1

    pedal_queue = queue.Queue()
    listener_queue = queue.Queue()
    writer_queue = queue.Queue()

    wthread = threading.Thread(target=writer_thread, args=(writer_queue,))
    lthread = threading.Thread(target=listener_thread, args=(listener_queue, writer_queue))
    pthread = threading.Thread(target=pedal_thread, args=(arduino, pedal_queue, listener_queue))

    wthread.start()
    lthread.start()
    pthread.start()

    try:
        wthread.join()
        lthread.join()
        pthread.join()
    except KeyboardInterrupt:
        listener_queue.put(Message(ListenerMessageType.STOP))
        writer_queue.put(Message(WriterMessageType.STOP))
        pedal_queue.put(Message(PedalMessageType.STOP))

    wthread.join()
    lthread.join()
    pthread.join()

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='you caN\'T coNVeY sArCASM tROugH WRitTeN tExT')
    parser.add_argument('--port', '-p', help='Port of the pedal', default='/dev/ttyACM0')
    args = parser.parse_args()

    port = args.port

    ret = main(port=port)

    exit(ret)
