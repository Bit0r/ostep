class Semaphore:
    def __init__(self, counter) -> None:
        from collections import deque
        self._counter = counter
        self._waiters = deque()

    def signal(self):
        from os import getpid
        self._counter -= 1
        if self._counter < 0:
            self._waiters.append(getpid())

    def wait(self):
        self._counter += 1
        if self._counter <= 0:
            self._waiters.popleft()
