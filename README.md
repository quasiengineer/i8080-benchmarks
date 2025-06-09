# i8080-benchmarks

Few benchmarks, adopted to be run on Intel 8080.

To compile them you need to have z88dk downloaded and you need to point `Z88DK_DIR` environment variable to folder with it.

Also `./configs/8080.cfg` should be copied to `z88dk\lib\config\` folder and `8080_crt.asm` should be copied to `z88dk\lib\target\8080\classic\`.

List of programs:
- `clocks`: sends lowest byte of tick counter to output device
- `clocks_hll`: prints value of tick counter
- `coremark`: CoreMark benchmark
- `dhrystone`: Dhrystone v2.1 benchmark
- `hello`: prints "Hello World!"
- `hello_asm`: prints "Hi!", minimal program
- `read_ram`: reads data from RAM and sends it to output device
- `write_ram`: stores data to RAM, then reads it back and sends value to output device