/*-
 * Copyright 2021 Vsevolod Stakhov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "css.h"
#include "css.hxx"
#include "css_style.hxx"

rspamd_css
rspamd_css_parse_style (const guchar *begin, gsize len, GError **err)
{
	rspamd::css::css_style_sheet *style = nullptr;


	return reinterpret_cast<rspamd_css>(style);
}

namespace rspamd::css {

INIT_LOG_MODULE_PUBLIC(css);

class css_style_sheet::impl {

};

css_style_sheet::css_style_sheet () : pimpl(new impl) {}
css_style_sheet::~css_style_sheet () {}

}