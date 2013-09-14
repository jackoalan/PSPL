#!/bin/sh
git submodule init
for i in $(git submodule | sed -e 's/.* //'); do
    spath=$(git config -f .gitmodules --get submodule.$i.path)
    surl=$(git config -f .gitmodules --get submodule.$i.url)
    sbranch=$(git config -f .gitmodules --get submodule.$i.branch)
    git clone -b $sbranch --depth 1 $surl $spath
done
git submodule update
