# Fuglidifications
An external memory editor for Roblox.
Functionality is very limited but written in C++, so while you get a headache you can custimize it as your liking.

# setup
git clone the repository then execute ```g++ src-main.cpp Backbone/Memory.cpp -o ffugl```.
for a simple gravity changer. Sets it to 0.

# notes
Functionality is very limited, like WalkSpeed wont work during my testing processess.
I did not test every offset since that would take too long, so take this project as beta.
I use the offsets from Theo. Theo's offsets at https://offsets.imtheo.lol/

# documentation
begin by getting the address pointer
```Cpp
Imem.point_addr(offsetParent, offsetClass);
```
example of
```Cpp
Instance x = Imem.point_addr(y, Offsets:z);
```
then overwrite it with the overwrite function
```Cpp
Imem.overwrite<type>(offsetAddress, typeValue);
```
typeValue must be set to the type. like float, 1.0f.
example of
```Cpp
Imem.overwrite<float>(offsetAddress, 1.0f);
```
to return the value of an address, use read
```Cpp
Imem.read<type>(objectAddress + Offset::x);
```
and type is the what the value is from the object address.
Example of featching the value
```Cpp
Imem.read<>(humanoidAddress + Offsets::Humanoid::Walkspeed);
```


<hr>

The source code copied from [Fries' Fugldifications](https://github.com/0x4672696573/Fuglidifications) and will no longer get updated from there.
