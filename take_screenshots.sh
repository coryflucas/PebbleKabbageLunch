mkdir -p screenshots
rm screenshots/*
for plat in `jq -r '.targetPlatforms[]' appinfo.json`
do
    pebble install --emulator $plat
    pebble screenshot --emulator $plat --no-open "screenshots/$plat.png"
done