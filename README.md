# bgp_simulator

A small BGP propagation simulator used for a course project. The simulator
models AS relationships (provider/customer/peer), seeds announcements, simulates
up/across/down propagation phases, and writes per-AS RIBs to CSV. It also
supports a simple ROV (Route Origin Validation) defense where ASes that
deploy ROV drop announcements marked as invalid.

## Build

Requirements:
- A C++17-capable compiler (g++ recommended)

From the project root run:

```bash
g++ -std=c++17 -I include src/ASGraph.cpp src/BGP.cpp src/main.cpp -o bgp_simulator
```

## Usage

The simulator executable accepts three files:

- `--relationships <path>`: relationship file with lines: `asn1|asn2|relationship`
  where `relationship` is `-1` for provider->customer (asn1 provider of asn2)
  or `0` for peers.
- `--announcements <path>`: CSV with header `seed_asn,prefix,rov_invalid`. Example:
  `1,10.0.0.0/24,False` or `2,1.2.0.0/16,True` where `rov_invalid` is `True`/`False`.
- `--rov-asns <path>`: file with one ASN per line indicating ASes that have
  ROV deployed.

Example:

```bash
.\bgp_simulator --relationships bench/many/CAIDAASGraphCollector_2025.10.16.txt \
  --announcements bench/many/anns.csv --rov-asns bench/many/rov_asns.csv
```

After running, the program writes `ribs.csv` in the current directory. The CSV
has header `asn,prefix,as_path` where `as_path` is formatted as a tuple like
`(4, 666)` or `(3,)`.

## Tests

There are small test programs under `tests/` (simple C++ binaries). To compile
and run a test, e.g. the output CSV test:

```powershell
g++ -std=c++17 -I include src/ASGraph.cpp src/BGP.cpp tests/test_output.cpp -o tests/run_output
.\tests\run_output
```

Other tests include `test_graph.cpp`, `test_conflicts.cpp`, `test_rov.cpp`, and
`test_ann_io.cpp` which validate graph building, conflict resolution, ROV
behavior, and announcements CSV parsing respectively.

## Key files

- `include/` — headers for `Announcement`, `ASGraph`, `ASNode`, `Policy`,
  `BGP`, and `ROV`.
- `src/ASGraph.cpp` — graph construction, propagation (up/across/down), CSV
  dump, CSV loaders for announcements and ROV lists.
- `src/BGP.cpp` — BGP policy implementation (local RIB, selection rules).
- `src/main.cpp` — CLI front-end that ties everything together and writes
  `ribs.csv`.
- `tests/` — small unit/integration test programs.

## Notes & Limitations

- This is an educational simulator with simplified BGP rules. It models
  relationship-based propagation and a minimal ROV defense, not a full BGP
  implementation.
- The announcements CSV loader expects exactly three columns per row
  (`seed_asn,prefix,rov_invalid`). `rov_invalid` should be `True` or `False`.
- The relationships file format is the same as CAIDA AS relationship files used
  in the `bench/` directory.

## Design Decisions

This section records the major design choices made while building the
simulator, the rationale behind them, and any important assumptions. It is
intended to help future readers understand why the code is structured the way
it is and where to safely extend it.

- Data model: Announcements are represented by the `Announcement` struct
  (`include/Announcement.h`) containing `prefix`, `as_path` (a
  `std::vector<uint32_t>`), `next_hop_asn`, `received_from` (a
  `Relationship` enum), and a boolean `rov_invalid` flag. This compact POD
  style keeps serialization and copying simple for tests and CSV output.

- AS nodes and policies: Each `ASNode` (`include/ASNode.h`) stores ASN lists
  for providers/customers/peers and owns a `std::unique_ptr<Policy>` called
  `policy`. `Policy` is an abstract interface implemented by `BGP` and
  `ROV`. Using a `unique_ptr` allows swapping policy implementations at
  runtime (e.g., enabling ROV for an AS) with minimal code changes.

- Policy inheritance for ROV: ROV is modeled as a subclass of `BGP` (`include/ROV.h`).
  `ROV::receiveAnnouncement` drops announcements whose `rov_invalid` flag is
  true before they reach the BGP selection logic. This choice favors code
  reuse (ROV reuses BGP storage and processing) and matches the simplified
  behavior requested in the assignment (drop invalid announcements immediately).

- Propagation model (up → across → down): The propagation algorithm in
  `ASGraph::propagateAnnouncements()` follows three phases: upward (customers
  advertise to providers), across (one-hop peer exchange), and downward
  (providers advertise to customers). This models common operational
  propagation constraints (providers learn customer routes first, peers
  exchange routes, then providers distribute to their other customers).

- Provider flattening and ranks: To implement the upward and downward
  phases efficiently and deterministically, the graph is flattened by
  provider/customer DAG using `flattenByProviders()` which assigns a
  `_propagation_rank` per node (nodes with no customers have rank 0). This
  lets the simulator iterate ranks in order rather than relying on event
  queues. The function detects provider cycles and raises an error because
  provider/customer cycles violate the DAG assumption and break rank-based
  propagation semantics.

- BGP selection rules: The `BGP` policy implements a simplified but
  deterministic selection:
  - Relationship precedence (origin > customer > peer > provider) is the
    primary criterion.
  - Shorter AS-path is preferred next.
  - Tie-break by lower `next_hop_asn`.
  This ordering was chosen to capture the assignment's rules (relationship
  precedence and AS-path length) while keeping the comparator simple and
  testable.

- Prepending ASN on store: When an AS processes announcements with
  `processAnnouncementsFor(my_asn)`, it prepends `my_asn` to the chosen
  announcement's AS-path before storing it in the local RIB. The selection
  functions take this into account by comparing `a.as_path.size() + 1` vs
  `b.as_path.size() + 1` to reflect the path length after the local prepend.

- Announcement forwarding: When a stored announcement is forwarded to a
  neighbor, the simulator forwards the stored `as_path` and preserves the
  `rov_invalid` flag. This ensures downstream ROV-deploying ASes can drop
  the announcement even if it originated elsewhere.
