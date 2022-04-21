#!/bin/bash
rm -f ./triggers.txt
echo "triggered at "`date` >> ./triggers.txt
git add ./triggers.txt
git commit -m "triggered"
presentBranch=`git status | grep "On branch" | sed "s/# //g" | sed "s/On branch //g" `
echo "triggering present branch= "${presentBranch}
git push origin ${presentBranch}



