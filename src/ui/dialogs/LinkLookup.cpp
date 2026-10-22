#include "ui/dialogs/LinkLookup.h"

#include <utility>

namespace westie::ui {

LinkLookup::LinkLookup(core::PlaylistLookup& lookup, const ToolBoard& tools) : lookup_(lookup), tools_(tools) {}

bool LinkLookup::set_link(const PlaylistLinkCheck& check) {
    // Another link to the same playlist, such as one with another ?si=, keeps what was found; only a failure is tried again
    if (link_ && check.link && link_->key() == check.link->key() && status_.state != LookupState::Failed) {
        return false;
    }

    lookup_.stop();
    link_ = check.link;
    status_ = {};
    if (!link_) {
        return false;
    }
    if (check.state == LinkState::AlreadyAdded && !check.added_name.empty()) {
        status_ = {.state = LookupState::Found, .name = check.added_name};
        return false;
    }
    status_.state = tools_available() ? LookupState::LookingUp : LookupState::WaitingForTools;
    return status_.state == LookupState::LookingUp;
}

void LinkLookup::start() {
    if (status_.state == LookupState::LookingUp) {
        lookup_.look_up(*link_);
    }
}

bool LinkLookup::tools_changed() {
    if (status_.state != LookupState::WaitingForTools || !tools_available()) {
        return false;
    }
    status_.state = LookupState::LookingUp;
    lookup_.look_up(*link_);
    return true;
}

bool LinkLookup::apply(const core::PlaylistLookedUp& answer) {
    // An answer for a link that was changed since comes too late
    if (status_.state != LookupState::LookingUp || answer.key != link_->key()) {
        return false;
    }
    switch (answer.outcome) {
    case core::PlaylistLookupOutcome::Found:       status_ = {.state = LookupState::Found, .name = answer.name}; break;
    case core::PlaylistLookupOutcome::Unavailable: status_ = {.state = LookupState::Unavailable}; break;
    case core::PlaylistLookupOutcome::ToolMissing: status_ = {.state = LookupState::WaitingForTools}; break;
    case core::PlaylistLookupOutcome::Failed:      status_ = {.state = LookupState::Failed, .error = answer.error}; break;
    }
    return true;
}

void LinkLookup::stop() noexcept {
    lookup_.stop();
}

const LookupStatus& LinkLookup::status() const noexcept {
    return status_;
}

bool LinkLookup::tools_available() const noexcept {
    // Only YouTube Music needs a tool for the look-up
    return link_->type != core::SourceType::YouTubeMusic || tools_.available(core::ToolPackage::YtDlp);
}

}
