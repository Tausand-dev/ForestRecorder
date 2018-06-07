with open("schedule.dat") as file:
    text = file.readlines()
    print(text)
    for (i, line) in enumerate(text):
        text[i] = line.replace("\r", "")
        if len(line) < 30:
            del text[i]

with open("schedule.dat", "w") as file:
    text = "".join(text)
    print(text)
    file.write(text)
