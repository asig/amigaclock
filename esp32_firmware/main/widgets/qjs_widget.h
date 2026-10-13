/*
 * This file is part of AmigaClock.
 *
 * Copyright (C) 2026 Andreas Signer <asigner@gmail.com>
 *
 * AmigaClock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AmigaClock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AmigaClock.  If not, see <https://www.gnu.org/licenses/>.
 */
 
#pragma once

#include "quickjs/quickjs.h"

#include "qjs_runtime.h"
#include "widgets/widget.h"

namespace widgets {

struct ClockDimensions;

class QjsWidget : public Widget {
public:
    QjsWidget();
    virtual ~QjsWidget();

    bool tick() override;
    void render() override;

    void set_property(const std::string &name, const std::string &value) override;
    std::string get_property(const std::string &name) const override;

    std::vector<std::string> get_property_names() const override;

private:
    JSContext *js_ctx;
};

} // namespace widgets
