#!/bin/bash

git submodule update --init ../include/memento/

git submodule update --init ../include/memento_expandable/
cd ../include/memento_expandable/ && git checkout expandable
cd -

git submodule update --init ../include/grafite/
git submodule update --init ../include/grafite/lib/sux/
git submodule update --init ../include/grafite/lib/sdsl-lite/

git submodule update --init ../include/surf/

git submodule update --init ../include/proteus/

git submodule update --init ../include/rencoder/

git submodule update --init ../include/snarf/

git submodule update --init ../include/oasis/
sed -i '$ d' ../include/oasis/CMakeLists.txt

