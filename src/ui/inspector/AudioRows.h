#pragma once

#include <functional>
#include <string>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/LinkLabel.h"
#include "ui/inspector/CardLayout.h"
#include "ui/inspector/ConfidenceValue.h"
#include "ui/inspector/LengthValue.h"
#include "ui/inspector/SectionTitle.h"
#include "ui/inspector/TrackDetails.h"

namespace westie::ui {

class AudioRows {
public:
    AudioRows(HWND card, std::function<void(const std::wstring&)> on_open_url);

    void apply_dpi(const framework::FontSet& fonts);
    void set_contents(const TrackDetails& track);
    void arrange(CardLayout& layout) const;

private:
    std::function<void(const std::wstring&)> on_open_url_;
    SectionTitle section_;
    framework::Label listed_key_;
    framework::LinkLabel listed_;
    framework::Label matched_key_;
    framework::Label matched_;
    framework::Label artists_key_;
    framework::Label artists_;
    framework::Label length_key_;
    LengthValue length_;
    framework::Label id_key_;
    framework::LinkLabel id_;
    framework::Label no_id_;
    framework::Label chosen_key_;
    framework::Label chosen_;
    framework::Label confidence_key_;
    ConfidenceValue confidence_;
    framework::Label spotify_match_key_;
    framework::Label spotify_match_;
    std::wstring id_url_;
    std::wstring listed_url_;
    bool has_id_ = false;
    bool has_listed_ = false;
    bool matcher_rows_ = false;
    bool spotify_match_row_ = false;
};

}
