INPUT_FILE = "data/live_l2_5d_30k.txt"
OUTPUT_FILE = "data/live_l2_5d_30k_with_color.txt"


def main():
    with open(INPUT_FILE, "r") as f:
        n = int(f.readline().strip())
        f.readline()          # Skip "5 Dims"

        points = []

        # Read all 5D points
        for _ in range(n):
            values = list(map(float, f.readline().split()))

            if len(values) != 5:
                raise ValueError(
                    f"Expected 5 coordinates, found {len(values)}"
                )

            points.append(values)

        # Remaining lines contain colors
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
        f.write("5 Dims + Color\n")

        for point, color in zip(points, colors):
            f.write(
                f"{point[0]:.6f} "
                f"{point[1]:.6f} "
                f"{point[2]:.6f} "
                f"{point[3]:.6f} "
                f"{point[4]:.6f} "
                f"{color}\n"
            )

    print(f"Generated: {OUTPUT_FILE}")
    print(f"Points: {n}")


if __name__ == "__main__":
    main()