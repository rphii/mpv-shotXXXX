# *use at your own risk*

# mpv-shotXXXX

Find `mpv-shotXXXX.jpg` files and move them all into one folder.

## Why? mpv has a config for where to store images!

- I still ended up with images in different locations, from when I didn't know about it
- so I don't have to manually sort rename them, and I was bored, I made this

## Building

```sh
git clone https://www.github.com/rphii/mpv-shotXXXX
cd mpv-shotXXXX/src && make
```

## Usage

- The input folder gets traversed recursively
- The output folder has to exist

```sh
./mpv-shotXXXX -i /path/to/your/input/folder -o /path/to/your/output/folder
```



