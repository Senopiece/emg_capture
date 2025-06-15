# Emg capture hardware

![Box showcase](fig/final_box.jpg)

## Steps to solder

1. Get up to 6 `Bioelectronic Circuit EMG` signal amplifiers

![amp](fig/amp.png)

2. Make a power supply for amplifiers
    - Get a `MEAN WELL GST25E18-P1J` 18v power supply 
    ![MEAN WELL GST25E18-P1J](fig/power18v.png)
    - Solder a `18v` - `+-9v` adapter ![schematic](fig/polar_converter.png)
    ![irl](fig/polar_converter_irl.jpg)
3. Flash `Seeduino XIAO nrf52` with [adc sampler](xiao_adc_sampler/src/main.c) and connect sEMG amplifiers to A0-A6 of the board

![xiao board](fig/xiao.png)

4. 3D print the [box](3d/box.plasticity) and put all the components altogether

![showcase](fig/box.jpg)

5. Optionally get a USB isolator like `BM8050usb`

![usb isolator](fig/usb_isolator.png)