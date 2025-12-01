#pragma once

#include "BGP.h"

// ROV policy: extends BGP by dropping any announcement marked as rov_invalid
class ROV : public BGP {
public:
    ROV() = default;

    void receiveAnnouncement(const Announcement& ann) override {
        if (ann.rov_invalid) {
            // Drop silently
            return;
        }
        BGP::receiveAnnouncement(ann);
    }
};
