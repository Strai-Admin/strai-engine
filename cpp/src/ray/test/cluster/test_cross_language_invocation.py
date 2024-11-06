import strai


@strai.remote
def py_return_input(v):
    return v


@strai.remote
def py_return_val():
    return 42


@strai.remote
class Counter(object):
    def __init__(self, value):
        self.value = int(value)

    def increase(self, delta):
        self.value += int(delta)
        return str(self.value)
