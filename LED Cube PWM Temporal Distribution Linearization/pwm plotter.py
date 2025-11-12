import matplotlib.pyplot as plt
import numpy as np

bits = 6
cycles = 2**bits
tShift = 7.7 #ms
tLayer = 2777.78 #ms
duty = 100#%
tOn = duty*tLayer/100
tBits = tLayer/cycles
OnBit = min(round(tOn / (tBits - tShift)), cycles)
OffBit = cycles - OnBit
print("Actual duty: ", OnBit*(tBits-tShift) / tLayer)
OnLessThanOff = OnBit <= OffBit

if OnLessThanOff:
    spread = 0 if not OnBit else cycles/OnBit
    halfspread = spread/2
else:
    spread = 0 if not OffBit else cycles/OffBit
    halfspread = spread/2

spread = round(spread)
halfspread = round(halfspread)
print(OnBit, OffBit, spread, halfspread)

x = np.array([str(i) for i in range(1,cycles+1)])
y = np.array([])
for i in range(1, cycles+1):
    if spread and (i - halfspread - 1) % spread == 0:
        bit = OnLessThanOff
    else:
        bit = not OnLessThanOff

    y = np.append(y, bit)

# print(x, '\n', y)
plt.bar(x,y)
plt.show()