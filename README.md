# Yang Example

I think yang is a good choice for things like config files and messages, and I'd love to see it used more widely. I couldn't find a simple example to build from anywhere, so I thought I'd whip one up.

So this makes a container with a simple example program using libyang to parse a json file and validate it against a schema.

The point is to have a simple working example to play with, in order to be able to try things, step through, etc. so that people who find this can more easily get yang working in their own systems.

As I learn more, I may expand on this a bit. If anybody's interested in adding a yang-example.py using pyang, I'd love a pull request.

# Running

```
docker build -t yex .
docker run -it --rm --cap-add=SYS_PTRACE --security-opt seccomp=unconfined yex
```

### Inside the container

```
valgrind ./yex
gdb ./yex
vi yang-example.c
./build.sh
```

# References

These are the main sources I used to bring this up. It was an hour or 3 to get it working.

 * Sample Data: <https://en.wikipedia.org/wiki/YANG#Example>
 * API docs: <https://github.com/CESNET/libyang/blob/master/src/libyang.h.in>
 * XPath: <https://www.w3schools.com/xml/xpath_syntax.asp>