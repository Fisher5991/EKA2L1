/*
 * Copyright (c) 2020 EKA2L1 Team.
 * 
 * This file is part of EKA2L1 project.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <epoc/services/etel/common.h>
#include <epoc/services/etel/etel.h>
#include <epoc/services/etel/phone.h>
#include <epoc/utils/err.h>
#include <epoc/epoc.h>

#include <common/cvt.h>

namespace eka2l1 {
    etel_server::etel_server(eka2l1::system *sys)
        : service::typical_server(sys, "!EtelServer") {
    }

    void etel_server::connect(service::ipc_context &ctx) {
        create_session<etel_session>(&ctx);
        ctx.set_request_status(epoc::error_none);
    }
    
    etel_session::etel_session(service::typical_server *serv, service::uid client_ss_uid, epoc::version client_ver)
        : service::typical_session(serv, client_ss_uid, client_ver) {
    }

    void etel_session::load_phone_module(service::ipc_context *ctx) {
        std::optional<std::u16string> name = ctx->get_arg<std::u16string>(0);

        if (!name) {
            ctx->set_request_status(epoc::error_argument);
            return;
        }

        if (!mngr_.load_tsy(ctx->sys->get_io_system(), common::ucs2_to_utf8(name.value()))) {
            ctx->set_request_status(epoc::error_already_exists);
            return;
        }

        ctx->set_request_status(epoc::error_none);
    }

    void etel_session::enumerate_phones(service::ipc_context *ctx) {
        std::uint32_t total_phone = static_cast<std::uint32_t>(mngr_.total_entries(epoc::etel_entry_phone));

        ctx->write_arg_pkg(0, total_phone);
        ctx->set_request_status(epoc::error_none);
    }

    void etel_session::get_phone_info_by_index(service::ipc_context *ctx) {
        epoc::etel_phone_info info;
        const std::int32_t index = *ctx->get_arg<std::int32_t>(1);
        std::optional<std::uint32_t> real_index = mngr_.get_entry_real_index(index, epoc::etel_entry_phone);

        if (!real_index.has_value()) {
            ctx->set_request_status(epoc::error_argument);
            return;
        }

        epoc::etel_module_entry *entry = nullptr;
        mngr_.get_entry(real_index.value(), &entry);

        etel_phone &phone = static_cast<etel_phone&>(*entry->entity_);

        ctx->write_arg_pkg<epoc::etel_phone_info>(0, phone.info_);
        ctx->set_request_status(epoc::error_none);
    }

    void etel_session::get_tsy_name(service::ipc_context *ctx) {
        const std::int32_t index = *ctx->get_arg<std::int32_t>(0);
        std::optional<std::uint32_t> real_index = mngr_.get_entry_real_index(index,
            epoc::etel_entry_phone);

        if (!real_index.has_value()) {
            ctx->set_request_status(epoc::error_argument);
            return;
        }

        epoc::etel_module_entry *entry = nullptr;
        mngr_.get_entry(real_index.value(), &entry);

        ctx->write_arg(1, common::utf8_to_ucs2(entry->tsy_name_));
        ctx->set_request_status(epoc::error_none);
    }

    void etel_session::open_from_session(service::ipc_context *ctx) {
        std::optional<std::u16string> name_of_object = ctx->get_arg<std::u16string>(0);

        if (!name_of_object) {
            ctx->set_request_status(epoc::error_argument);
            return;
        }

        LOG_TRACE("Opening {} from session", common::ucs2_to_utf8(name_of_object.value()));

        epoc::etel_module_entry *entry = nullptr;
        if (!mngr_.get_entry_by_name(common::ucs2_to_utf8(name_of_object.value()), &entry)) {
            ctx->set_request_status(epoc::error_not_found);
            return;
        }

        // Check for empty slot in the subessions array
        auto empty_slot = std::find(subsessions_.begin(), subsessions_.end(), nullptr);
        std::unique_ptr<etel_subsession> subsession;

        switch (entry->entity_->type()) {
        case epoc::etel_entry_phone:
            subsession = std::make_unique<etel_phone_subsession>(this, reinterpret_cast<etel_phone*>(
                entry->entity_.get()));

            break;

        default:
            LOG_ERROR("Unsupported entry type of etel module {}", static_cast<int>(entry->entity_->type()));
            ctx->set_request_status(epoc::error_general);
            return;
        }

        if (empty_slot == subsessions_.end()) {
            subsessions_.push_back(std::move(subsession));
            ctx->write_arg_pkg<std::uint32_t>(3, static_cast<std::uint32_t>(subsessions_.size()));
        } else {
            *empty_slot = std::move(subsession);
            ctx->write_arg_pkg<std::uint32_t>(3, static_cast<std::uint32_t>(std::distance(subsessions_.begin(),
                empty_slot) + 1));
        }

        ctx->set_request_status(epoc::error_none);
    }

    void etel_session::open_from_subsession(service::ipc_context *ctx) {
        LOG_TRACE("Open from etel subsession stubbed");

        const std::uint32_t dummy_handle = 0x43210;
        ctx->write_arg_pkg<std::uint32_t>(3, dummy_handle);
        ctx->set_request_status(epoc::error_none);
    }

    void etel_session::close_sub(service::ipc_context *ctx) {
        std::optional<std::uint32_t> subhandle = ctx->get_arg<std::uint32_t>(3);

        if (subhandle && subhandle.value() <= subsessions_.size()) {
            subsessions_[subhandle.value() - 1].reset();
        }

        ctx->set_request_status(epoc::error_none);
    }

    void etel_session::query_tsy_functionality(service::ipc_context *ctx) {
        LOG_TRACE("Query TSY functionality stubbed");
        ctx->set_request_status(epoc::error_none);
    }

    void etel_session::fetch(service::ipc_context *ctx) {
        switch (ctx->msg->function) {
        case epoc::etel_open_from_session:
            open_from_session(ctx);
            break;

        case epoc::etel_open_from_subsession:
            open_from_subsession(ctx);
            break;

        case epoc::etel_close:
            close_sub(ctx);
            break;

        case epoc::etel_enumerate_phones:
            enumerate_phones(ctx);
            break;

        case epoc::etel_get_tsy_name:
            get_tsy_name(ctx);
            break;

        case epoc::etel_phone_info_by_index:
            get_phone_info_by_index(ctx);
            break;

        case epoc::etel_load_phone_module:
            load_phone_module(ctx);
            break;

        case epoc::etel_query_tsy_functionality:
            query_tsy_functionality(ctx);
            break;

        default:
            std::optional<std::uint32_t> subsess_id = ctx->get_arg<std::uint32_t>(3);
            
            if (subsess_id && (subsess_id.value() > 0)) {
                if (subsess_id.value() <= subsessions_.size()) {
                    subsessions_[subsess_id.value() - 1]->dispatch(ctx);
                    return;
                }
            }

            LOG_ERROR("Unimplemented ETel server opcode {}", ctx->msg->function);
            break;
        }
    }
}