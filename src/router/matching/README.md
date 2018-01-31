
## Standalone CBR filter engine 

No message forwarding.
Used as a library by SCBR.
`main.cc` is used only for testing purposes.


# USING THE CODE

Build application (boost required)
```
  make
```

Run application
```
  ./cbr -h
  ./cbr --verbose -c subs/s10k pubs/p5k
  ./cbr --verbose -p -v 0 subs/s10k pubs/p5k
  ./cbr --verbose -p -v 1 subs/s10k pubs/p5k
  ./cbr --verbose -p -v 2 subs/s10k pubs/p5k
  ./cbr --verbose -p -v 0 -b 1024 -n 5 subs/s10k pubs/p5k
```

# NOTES

We need subscriptions with several equality constraints for pre-filtering to be efficient.

