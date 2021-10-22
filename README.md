
![pixmap565_256](https://user-images.githubusercontent.com/48536508/137931703-b3fb470f-9695-48c0-bed1-890e0eb8e7ef.png)

A utility to convert between BMP565 picture and RGB565 pixmap.
These pixmaps could be used in embedded systems like Makerbase MKS-TFT.

## Dependencies:
- GNU make

## Download:
```
git clone https://github.com/plerros/pixmap565.git
```
## Compile:
#### GNU/Linux, HaikuOS, Homebrew, WSL
```
make
```
#### FreeBSD, OpenIndiana
```
gmake
```
## Run:
```
./pixmap565 -i infile.bmp -o outfile
./pixmap565 -w width -i infile -o outfile.bmp
```
## Scripts:
```
./mkstft28.sh infile outfile
./mkstft28-icon.sh infile outfile
```
