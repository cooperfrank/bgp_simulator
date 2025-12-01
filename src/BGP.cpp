#include "BGP.h"

void BGP::processAnnouncements() {
    auto score_rel = [](Relationship r) {
        switch (r) {
            case Relationship::Origin: return 3;
            case Relationship::Customer: return 2;
            case Relationship::Peer: return 1;
            case Relationship::Provider: return 0;
        }
        return 0;
    };

    auto better = [&](const Announcement &a, const Announcement &b) {
        // return true if a is preferred to b
        int sa = score_rel(a.received_from);
        int sb = score_rel(b.received_from);
        if (sa != sb) return sa > sb;
        if (a.as_path.size() != b.as_path.size()) return a.as_path.size() < b.as_path.size();
        return a.next_hop_asn < b.next_hop_asn;
    };

    for (auto& [prefix, announcements] : received_queue) {
        if (announcements.empty()) continue;

        // pick best among received announcements
        size_t best_idx = 0;
        for (size_t i = 1; i < announcements.size(); ++i) {
            if (better(announcements[i], announcements[best_idx])) best_idx = i;
        }

        const Announcement &chosen = announcements[best_idx];

        auto it = local_rib.find(prefix);
        if (it == local_rib.end()) {
            // no existing route, store chosen as-is
            local_rib.insert_or_assign(prefix, chosen);
        } else {
            // compare chosen to existing stored announcement; store if better
            if (better(chosen, it->second)) {
                local_rib.insert_or_assign(prefix, chosen);
            }
        }
    }
    // Clear the received queue after processing
    received_queue.clear();
}

void BGP::processAnnouncementsFor(uint32_t my_asn) {
    for (auto& [prefix, announcements] : received_queue) {
        if (announcements.empty()) continue;

        // Choose the best announcement according to rules (relationship > path length > next hop)
        auto score_rel2 = [](Relationship r) {
            switch (r) {
                case Relationship::Origin: return 3;
                case Relationship::Customer: return 2;
                case Relationship::Peer: return 1;
                case Relationship::Provider: return 0;
            }
            return 0;
        };

        auto better2 = [&](const Announcement &a, const Announcement &b) {
            int sa = score_rel2(a.received_from);
            int sb = score_rel2(b.received_from);
            if (sa != sb) return sa > sb;
            size_t a_len = a.as_path.size() + 1; // after prepending my_asn
            size_t b_len = b.as_path.size() + 1;
            if (a_len != b_len) return a_len < b_len;
            return a.next_hop_asn < b.next_hop_asn;
        };

        size_t best_idx = 0;
        for (size_t i = 1; i < announcements.size(); ++i) {
            if (better2(announcements[i], announcements[best_idx])) best_idx = i;
        }

        // Build the stored announcement: prepend my_asn to AS path
        Announcement chosen = announcements[best_idx];
        std::vector<uint32_t> new_path;
        new_path.reserve(chosen.as_path.size() + 1);
        new_path.push_back(my_asn);
        new_path.insert(new_path.end(), chosen.as_path.begin(), chosen.as_path.end());

        Announcement stored(prefix, chosen.next_hop_asn, chosen.received_from, new_path, chosen.rov_invalid);

        // Compare with existing local RIB entry (if any) and only replace if better
        auto it2 = local_rib.find(prefix);
        if (it2 == local_rib.end()) {
            local_rib.insert_or_assign(prefix, stored);
        } else {
            if (better2(stored, it2->second)) {
                local_rib.insert_or_assign(prefix, stored);
            }
        }
    }

    received_queue.clear();
}