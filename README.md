# Conway’s Game of Life — **linked list** vs **ring buffer** comparison

This root contains **two separate C11 projects** to compare the performance of two history data structures:

- `projet-listechainee/`: history via a **doubly linked list** (baseline)
- `projet-ringbuffer/`: history via a bounded **ring buffer** (optimized)
- `bench/`: comparative benchmark script

## Restructuring commands (reference)

If you had to recreate the structure starting from a directory containing a single project (indicative):

```bash
mkdir -p projet-listechainee projet-ringbuffer bench
# then move/copy the project to projet-listechainee and duplicate to projet-ringbuffer
```

## Dependencies

On Debian/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y build-essential libsdl2-dev
```

## Build / Run (SDL2 UI)

### Linked list

```bash
make -C projet-listechainee
./projet-listechainee/bin/life --input projet-listechainee/data/glider.txt --history-cap 512
```

### Ring buffer

```bash
make -C projet-ringbuffer
./projet-ringbuffer/bin/life --input projet-ringbuffer/data/glider.txt --history-cap 512
```

## Read from a file + save after N iterations (batch mode, no SDL)

The program reads the initial grid from `--input`, computes `--steps N` generations, then writes the final grid to `--output`.

### Linked list

```bash
make -C projet-listechainee
./projet-listechainee/bin/life --input projet-listechainee/data/glider.txt --steps 200 --output out_list.txt --history-cap 512
```

### Ring buffer

```bash
make -C projet-ringbuffer
./projet-ringbuffer/bin/life --input projet-ringbuffer/data/glider.txt --steps 200 --output out_ring.txt --history-cap 512
```

## Comparative benchmark

The script builds both projects (bench mode, `-O3`) then runs both benchmarks with **exactly** the same parameters.

```bash
chmod +x bench/bench.sh
./bench/bench.sh --width 512 --height 512 --steps 2000 --seed 42 --history-cap 512
```

Each bench binary prints a single `RESULT ...` line (stable parsing), then `bench.sh` prints a summary and the winner.
