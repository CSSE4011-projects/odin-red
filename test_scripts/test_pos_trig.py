import math
import matplotlib.pyplot as plt

Y_MAX_CM = 200
Y_MIN_CM = 0
X_MAX_CM = 200
X_MIN_CM = 0

THRESHOLD = 1

def main():
    plt.axes()
    rectangle = plt.Rectangle((0,0), 200, 200, fc="None", ec="red")
    plt.gca().add_patch(rectangle)
    plt.axis('scaled')

    x = 20
    y = 50
    angle = 60
    plt.scatter(x, y, c='blue')
    for x in range(1,200):
        for y in range(1,200):
            for angle in range(360):
                pred_x, pred_y = simulate(angle, x, y)
                if abs(pred_x - x) > 5 or abs(pred_y - y) > 5:
                    print(f"x: {x}, bad angle: {angle}")
                    plt.scatter(pred_x, pred_y, c='red')
    # pred_x, pred_y = simulate(angle, x, y)
    # plt.scatter(pred_x, pred_y, c='red')

    plt.show()

def simulate(angle, x, y):

    front_angle = (angle % 360) * math.pi/180.0
    back_angle = ((angle + 180) % 360) * math.pi/180.0
    left_angle = ((angle + 90) % 360) * math.pi/180.0
    right_angle = ((angle - 90) % 360) * math.pi/180.0

    fd = get_distance(front_angle, x, y)
    bd = get_distance(back_angle, x, y)
    ld = get_distance(left_angle, x, y)
    rd = get_distance(right_angle, x, y)

    # print(f"fd: {fd}, bd: {bd}, ld: {ld}, rd: {rd}, angle: {angle}")

    return find_position(angle, fd, bd, ld, rd)

def get_distance(angle, x, y):

    angle_deg = angle * 180.0 / math.pi

    if angle_deg == 0:
        return X_MAX_CM - x
    elif angle_deg == 90:
        return Y_MAX_CM - y
    elif angle_deg == 180:
        return x
    elif angle_deg == 270:
        return y
    
    if 0 < angle_deg < 180:
        y_ref = Y_MAX_CM - y
    else:
        y_ref = Y_MIN_CM - y
    
    if 90 < angle_deg < 270:
        x_ref = X_MIN_CM - x
    else:
        x_ref = X_MAX_CM - x

    distance1 = y_ref
    distance2 = x_ref

    if angle_deg % 180 != 0:
        distance1 = y_ref / math.sin(angle)
    
    if (angle_deg + 90) % 180 != 0:
        distance2 = x_ref / math.cos(angle)
        

    return min(distance1, distance2)

# Takes in angle in degrees, and distance in m
def find_position(angle, front_d, back_d, left_d, right_d):

    front_angle = (angle % 360) * math.pi/180.0
    back_angle = ((angle + 180) % 360) * math.pi/180.0
    left_angle = ((angle + 90) % 360) * math.pi/180.0
    right_angle = ((angle - 90) % 360) * math.pi/180.0

    x_front, y_front = find_solution_lines(front_d, front_angle)
    x_back, y_back = find_solution_lines(back_d, back_angle)
    x_left, y_left = find_solution_lines(left_d, left_angle)
    x_right, y_right = find_solution_lines(right_d, right_angle)
    # print(f"x_front: {x_front}, y_front: {y_front}")
    # print(f"x_back: {x_back}, y_back: {y_back}")
    # print(f"x_left: {x_left}, y_left: {y_left}")
    # print(f"x_right: {x_right}, y_right: {y_right}")

    # TODO: recalculate to find where all four lines intersect
    ##########################################################

    xs = [x_front, x_back, x_left, x_right]
    ys = [y_front, y_back, y_left, y_right]
    
    for i in range(4):
        j = (i + 1) % 4
        k = (i + 2) % 4
        l = (i + 3) % 4
        if abs(xs[i] - xs[j]) < THRESHOLD and abs(xs[i] - xs[k]) < THRESHOLD:
            if (xs[i] != 0 and ys[l] != 0):
                return xs[i], ys[l] # TODO: Could return the average instead of choosing one of the similar ones
        if abs(ys[i] - ys[j]) < THRESHOLD and abs(ys[i] - ys[k]) < THRESHOLD:
            if (xs[l] != 0 and ys[i] != 0):
                return xs[l], ys[i]
    
    for i in range(4):
        for j in range(i + 1, 4):
            if i == 0 and j == 1:
                k = 2
                l = 3
            elif i == 0 and j == 2:
                k = 1
                l = 3
            elif i == 0 and j == 3:
                k = 1
                l = 2
            elif i == 1 and j == 2:
                k = 0
                l = 3
            elif i == 1 and j == 3:
                k = 0
                l = 2
            elif i == 2 and j == 3:
                k = 0
                l = 1
            
            if abs(xs[i] - xs[j]) < THRESHOLD and abs(ys[k] - ys[l]) < THRESHOLD:
                if (xs[i] != 0 and ys[k] != 0):
                    return xs[i], ys[k]


    ##########################################################
    
    return 0, 0
    

def find_solution_lines(distance, angle):
    yd = distance * math.sin(angle)
    xd = distance * math.cos(angle)

    angle_deg = angle * 180.0 / math.pi


    if angle_deg == 0:
        return X_MAX_CM - xd, 0
    elif angle_deg == 90:
        return 0, Y_MAX_CM - yd
    elif angle_deg == 180:
        return X_MIN_CM - xd, 0
    elif angle_deg == 270:
        return 0, Y_MIN_CM - yd
    
    if 0 < angle_deg < 180:
        y = Y_MAX_CM - yd
    else:
        y = Y_MIN_CM - yd
    
    if 90 < angle_deg < 270:
        x = X_MIN_CM - xd
    else:
        x = X_MAX_CM - xd
    
    return x, y

if __name__ == "__main__":
    main()