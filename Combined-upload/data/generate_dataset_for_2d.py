INPUT_FILE = "data/live_l2_5d_30k.txt"
OUTPUT_FILE = "data/live_l2_2d_30k.txt"

DIM_X = 0
DIM_Y = 1


def main():
    with open(INPUT_FILE, "r") as f:
        n = int(f.readline().strip())
        f.readline()  # skip "5 Dims"

        points = []

        for _ in range(n):
            values = list(map(float, f.readline().split()))
            points.append(values)

        # Current source format: all colors on final line
        colors = []

        for line in f:
            colors.extend(line.split())


    if len(points) != n:
        raise ValueError(
            f"Expected {n} points, found {len(points)}"
        )

    if len(colors) != n:
        raise ValueError(
            f"Expected {n} colors, found {len(colors)}"
        )


    with open(OUTPUT_FILE, "w") as f:
        f.write(f"{n}\n")
        f.write("2 Dims + Color\n")

        for point, color in zip(points, colors):
            x = point[DIM_X]
            y = point[DIM_Y]

            f.write(
                f"{x:.6f} {y:.6f} {color}\n"
            )


    print("Generated:", OUTPUT_FILE)
    print("Points:", n)
    print("Dimensions selected:", DIM_X, DIM_Y)


if __name__ == "__main__":
    main()