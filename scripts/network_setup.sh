export DEV="lo"
export SPEED="1Gbit"
export BURST="100000"
LAT="${LAT:="0msec"}"

echo "Setting ${DEV} to ${SPEED} with latency ${LAT}"

sudo tc qdisc del dev $DEV root
sudo tc qdisc add dev $DEV root handle 1: tbf rate ${SPEED} burst ${BURST} limit ${BURST}
sudo tc qdisc add dev $DEV parent 1:1 handle 10: netem delay ${LAT}
