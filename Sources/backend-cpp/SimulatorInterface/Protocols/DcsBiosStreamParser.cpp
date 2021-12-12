// Copyright 2021 Charles Tytler, modified from "ExportStreamParser":
/*
    Copyright 2015 Craig Courtney
    This file is part of DcsBios-Firmware.
    DcsBios-Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    DcsBios-Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with DcsBios-Firmware.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DcsBiosStreamParser.h"

DcsBiosStreamParser::DcsBiosStreamParser()
{
    _state = DcsBiosState::WAIT_FOR_SYNC;
    _sync_byte_count = 0;
}

bool DcsBiosStreamParser::processByte(uint8_t c, std::unordered_map<unsigned int, unsigned int> &data_by_address)
{
    bool end_of_frame_sync_received = false;

    switch (_state) {
    case DcsBiosState::WAIT_FOR_SYNC:
        /* do nothing */
        break;

    case DcsBiosState::ADDRESS_LOW:
        _address = (unsigned int)c;
        _state = DcsBiosState::ADDRESS_HIGH;
        break;

    case DcsBiosState::ADDRESS_HIGH:
        _address = (c << 8u) | _address;
        if (_address != 0x5555) {
            _state = DcsBiosState::COUNT_LOW;
        } else {
            _state = DcsBiosState::WAIT_FOR_SYNC;
        }
        break;

    case DcsBiosState::COUNT_LOW:
        _count = (unsigned int)c;
        _state = DcsBiosState::COUNT_HIGH;
        break;

    case DcsBiosState::COUNT_HIGH:
        _count = (c << 8u) | _count;
        _state = DcsBiosState::DATA_LOW;
        break;

    case DcsBiosState::DATA_LOW:
        _data = (unsigned int)c;
        _count--;
        _state = DcsBiosState::DATA_HIGH;
        break;

    case DcsBiosState::DATA_HIGH:
        _data = (c << 8u) | _data;
        _count--;
        data_by_address[_address] = _data;
        if (_count == 0) {
            _state = DcsBiosState::ADDRESS_LOW;

            // Frame sync moved to end of frame.All time consuming updates should
            // be handled in framesync during the down time between frame transmissions.
            if (_address == 0xfffe) {
                end_of_frame_sync_received = true;
            }
        } else {
            _address += 2;
            _state = DcsBiosState::DATA_LOW;
        }
        break;
    }

    if (c == 0x55) {
        _sync_byte_count++;
    } else {
        _sync_byte_count = 0;
    }

    if (_sync_byte_count == 4) {
        _state = DcsBiosState::ADDRESS_LOW;
        _sync_byte_count = 0;
    }

    return end_of_frame_sync_received;
}
