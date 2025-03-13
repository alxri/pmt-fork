import time
import pmt

platform = "rapl"

@pmt.measure(platform)
def my_kernel1():
    time.sleep(1)


def my_kernel2():
    pm = pmt.create(platform)
    start = pm.read()
    time.sleep(1)
    end = pm.read()
    return {
        "platform": platform,
        "joules": format(pmt.joules(start, end), ".3f"),
        "seconds": format(pmt.seconds(start, end), ".3f"),
        "watt": format(pmt.watts(start, end), ".3f"),
    }


def my_kernel3():
    pm = pmt.create(platform)
    state = pm.read()
    return state.measurements()

if __name__ == "__main__":
    print(my_kernel1())
    print(my_kernel2())
    print(my_kernel3())
