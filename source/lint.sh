#!/bin/sh
find apps -name '*.[ch]' -exec cpplint '{}' +
