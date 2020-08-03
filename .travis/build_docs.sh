#! /bin/bash

pip install sphinx
echo $(if [[ "$TRAVIS_BRANCH" == "develop" || "$TRAVIS_BRANCH" == "master" ]];
then echo "$TRAVIS_BRANCH"; elif [[ ! -z "$TRAVIS_TAG" ]]; then echo "tags/$TRAVIS_TAG";
else echo "branches/$(echo $TRAVIS_BRANCH | tr / _ | tr - .)"; fi) | tee /tmp/output_location
cd documentation
make html
ls ./build/html
mv ./build/html /tmp/build.html
cd ..
git checkout gh-pages
git clean -d -f -x
rm -rf SimpleAmqpClient
rm -rf scarab
ls
mkdir -p ./$(cat /tmp/output_location)
rsync -av --delete /tmp/build.html/ ./$(cat /tmp/output_location)
tree -L 3
git add $(cat /tmp/output_location)
git status
git diff --cached --quiet; export HAVE_STAGED_FILES=$?
echo $HAVE_STAGED_FILES
if [[ "$HAVE_STAGED_FILES" != "0" ]]; then
git commit -m "built docs for ${TRAVIS_BRANCH}"
git remote -v
git remote set-url origin "git@github.com:${TRAVIS_REPO_SLUG}"
git remote -v
git push
else
echo "No documentation updates to push"
fi
