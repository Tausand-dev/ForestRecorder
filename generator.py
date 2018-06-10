from datetime import timedelta
from datetime import datetime

start = []
duration = []

while True:
    while True:
        s = input("Start date (dd/mm/yy): ")
        if len(s) == 8:
            break
    while True:
        s_ = input("Start time (hh:mm): ")
        if len(s_) == 5:
            s = s + " " + s_
            break
    while True:
        d = input("Duration (minutes): ")
        try:
            d = int(d)
            break
        except ValueError:
            pass

    s = s.replace(" ", "-").replace("/", "-").replace(":", "-")
    next = datetime.strptime(s, '%d-%m-%y-%H-%M') + timedelta(minutes = d)
    next = next.strftime("%d-%m-%y-%H-%M")

    with open("schedule.dat", "a") as file:
        file.write(s + "; " + next)
