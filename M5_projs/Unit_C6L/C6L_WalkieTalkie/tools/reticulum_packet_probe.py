#!/usr/bin/env python3
import argparse
import os
import sys
import time

import RNS

APP_NAME = "c6ltest"
ASPECT = "echo"


def load_identity(configdir):
    identity_dir = os.path.join(configdir, "storage", "identities")
    os.makedirs(identity_dir, exist_ok=True)
    identity_path = os.path.join(identity_dir, "c6ltest_echo")
    identity = RNS.Identity.from_file(identity_path) if os.path.isfile(identity_path) else None
    if identity is None:
        identity = RNS.Identity()
        identity.to_file(identity_path)
    return identity


def print_packet_stats(prefix, packet, reticulum=None):
    rssi = getattr(packet, "rssi", None)
    snr = getattr(packet, "snr", None)
    if reticulum is not None and packet is not None and reticulum.is_connected_to_shared_instance:
        rssi = reticulum.get_packet_rssi(packet.packet_hash)
        snr = reticulum.get_packet_snr(packet.packet_hash)

    stats = []
    if rssi is not None:
        stats.append(f"RSSI={rssi} dBm")
    if snr is not None:
        stats.append(f"SNR={snr} dB")
    print(f"{prefix}: " + (", ".join(stats) if stats else "RSSI/SNR unavailable"), flush=True)


def listen(args):
    RNS.Reticulum(configdir=args.config, loglevel=3 + args.verbose)
    identity = load_identity(args.config)
    destination = RNS.Destination(identity, RNS.Destination.IN, RNS.Destination.SINGLE, APP_NAME, ASPECT)
    destination.set_proof_strategy(RNS.Destination.PROVE_ALL)

    def on_packet(data, packet):
        print(f"received {len(data)} bytes from {RNS.prettyhexrep(packet.destination_hash)}", flush=True)
        print_packet_stats("rx packet", packet)

    destination.set_packet_callback(on_packet)
    print(destination.hash.hex(), flush=True)
    destination.announce()
    last_announce = time.time()

    while True:
        time.sleep(0.2)
        if args.announce_interval > 0 and time.time() - last_announce >= args.announce_interval:
            destination.announce()
            last_announce = time.time()


def send(args):
    reticulum = RNS.Reticulum(configdir=args.config, loglevel=3 + args.verbose)
    destination_hash = bytes.fromhex(args.destination)

    if not RNS.Transport.has_path(destination_hash):
        RNS.Transport.request_path(destination_hash)
        deadline = time.time() + args.timeout
        while not RNS.Transport.has_path(destination_hash) and time.time() < deadline:
            time.sleep(0.1)

    if not RNS.Transport.has_path(destination_hash):
        print("path not found", file=sys.stderr)
        return 2

    identity = RNS.Identity.recall(destination_hash)
    if identity is None:
        print("destination identity not recalled", file=sys.stderr)
        return 2

    destination = RNS.Destination(identity, RNS.Destination.OUT, RNS.Destination.SINGLE, APP_NAME, ASPECT)
    payload = bytes((i * 17 + 23) % 256 for i in range(args.size))
    packet = RNS.Packet(destination, payload)
    sent_at = time.time()
    receipt = packet.send()
    deadline = time.time() + args.timeout
    while receipt.status == RNS.PacketReceipt.SENT and time.time() < deadline:
        time.sleep(0.1)

    if receipt.status != RNS.PacketReceipt.DELIVERED:
        print("probe proof timed out", file=sys.stderr)
        return 1

    print(f"delivered {args.size} bytes in {time.time() - sent_at:.3f}s", flush=True)
    if receipt.proof_packet is not None:
        print_packet_stats("proof packet", receipt.proof_packet, reticulum)
    else:
        print("proof packet: unavailable", flush=True)
    return 0


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="mode", required=True)

    listen_parser = sub.add_parser("listen")
    listen_parser.add_argument("--config", required=True)
    listen_parser.add_argument("--announce-interval", type=float, default=10.0)
    listen_parser.add_argument("-v", "--verbose", action="count", default=0)

    send_parser = sub.add_parser("send")
    send_parser.add_argument("--config", required=True)
    send_parser.add_argument("--destination", required=True)
    send_parser.add_argument("--size", type=int, default=32)
    send_parser.add_argument("--timeout", type=float, default=90.0)
    send_parser.add_argument("-v", "--verbose", action="count", default=0)

    args = parser.parse_args()
    if args.mode == "listen":
        listen(args)
    elif args.mode == "send":
        raise SystemExit(send(args))


if __name__ == "__main__":
    main()
