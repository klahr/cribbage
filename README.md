# Cribbage

Two-player cribbage against a bot for Sailfish OS.

- Full rules: cut for deal, laying away to the crib, the play with fifteens, pairs, runs,
  go and 31, and the show of both hands and the crib, all counted for you
- Each score is shown one at a time on the cards that made it, at a speed you choose
- A bot in three levels: Easy plays at random, Medium keeps the best hand, Hard weighs
  every possible starter and the crib and plays to deny you points
- A peg board with front and back pegs and skunk lines, and won and lost games per level

Needs Sailfish OS 5.1 or newer, on aarch64 or armv7hl.

## Building

With the [Sailfish SDK](https://docs.sailfishos.org/Tools/Sailfish_SDK/), from the source directory:

```
sfdk -c target=SailfishOS-5.1.0.11-aarch64 build
sfdk -c target=SailfishOS-5.1.0.11-armv7hl build
```

The RPM ends up in `RPMS/`. Clean the build files between targets.

The rules and the bot are plain C++ without Qt, with tests that run on any desktop:

```
make -C tests
```

They also run when the RPM is built.

## License

GPLv3, see [LICENSE](LICENSE).
