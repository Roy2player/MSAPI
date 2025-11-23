/**************************
 * @file        html.inl
 * @version     6.0
 * @date        2025-11-20
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2025 Maksim Andreevich Leonov
 *
 * This file is part of MSAPI.
 * License: see LICENSE.md
 * Contributor terms: see CONTRIBUTING.md
 *
 * This software is licensed under the Polyform Noncommercial License 1.0.0.
 * You may use, copy, modify, and distribute it for noncommercial purposes only.
 *
 * For commercial use, please contact: maks.angels@mail.ru
 *
 * Required Notice: MSAPI, copyright © 2021–2025 Maksim Andreevich Leonov, maks.angels@mail.ru
 */

#ifndef MSAPI_UNIT_TEST_HTML_INL
#define MSAPI_UNIT_TEST_HTML_INL

#include "../../../../library/source/help/html.h"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for HTML.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool HTML();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool HTML()
{
	LOG_INFO_UNITTEST("MSAPI HTML");
	MSAPI::Test t;

	const std::string_view justHtml{
		"<html><head></head><body><header></header><main><section></section></main><footer></footer></body></html>"
	};

	MSAPI::HTML html(justHtml);
	LOG_DEBUG(html.ToString());

	RETURN_IF_FALSE(t.Assert(html.MaxDepth(), 4, "HTML Depth"));
	RETURN_IF_FALSE(t.Assert(html.BodySize(), justHtml.size(), "HTML Size"));
	RETURN_IF_FALSE(t.Assert(html.TagsSize(), 14, "HTML tags size"));
	RETURN_IF_FALSE(t.Assert(html.GetTag(), html.GetTag(html.TagsSize() + 1), "HTML get default tag"));

	const auto checkTag{ [&t, &html] [[nodiscard]] (const size_t index, const size_t begin, const size_t end,
							 const uint depth, const MSAPI::HTML::Valid isOpenTag, const MSAPI::HTML::Type type,
							 const MSAPI::HTML::Valid valid) {
		const auto& teg = html.GetTag(index);
		RETURN_IF_FALSE(t.Assert(teg.begin, begin, std::format("HTML Tag, begin. Id: {}", index)));
		RETURN_IF_FALSE(t.Assert(teg.end, end, std::format("HTML Tag, end. Id: {}", index)));
		RETURN_IF_FALSE(t.Assert(teg.depth, depth, std::format("HTML Tag, depth. Id: {}", index)));
		RETURN_IF_FALSE(t.Assert(teg.isOpenTag, isOpenTag, std::format("HTML Tag, is open. Id: {}", index)));
		RETURN_IF_FALSE(t.Assert(teg.type, type, std::format("HTML Tag, type. Id: {}", index)));
		RETURN_IF_FALSE(t.Assert(teg.valid, valid, std::format("HTML Tag, valid. Id: {}", index)));
		return true;
	} };

	RETURN_IF_FALSE(checkTag(1, 0, 5, 1, MSAPI::HTML::Valid::True, MSAPI::HTML::Type::Html, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(checkTag(2, 6, 11, 2, MSAPI::HTML::Valid::True, MSAPI::HTML::Type::Head, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(3, 12, 18, 2, MSAPI::HTML::Valid::False, MSAPI::HTML::Type::Head, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(4, 19, 24, 2, MSAPI::HTML::Valid::True, MSAPI::HTML::Type::Body, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(5, 25, 32, 3, MSAPI::HTML::Valid::True, MSAPI::HTML::Type::Header, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(6, 33, 41, 3, MSAPI::HTML::Valid::False, MSAPI::HTML::Type::Header, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(7, 42, 47, 3, MSAPI::HTML::Valid::True, MSAPI::HTML::Type::Main, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(8, 48, 56, 4, MSAPI::HTML::Valid::True, MSAPI::HTML::Type::Section, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(9, 57, 66, 4, MSAPI::HTML::Valid::False, MSAPI::HTML::Type::Section, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(10, 67, 73, 3, MSAPI::HTML::Valid::False, MSAPI::HTML::Type::Main, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(11, 74, 81, 3, MSAPI::HTML::Valid::True, MSAPI::HTML::Type::Footer, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(12, 82, 90, 3, MSAPI::HTML::Valid::False, MSAPI::HTML::Type::Footer, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(13, 91, 97, 2, MSAPI::HTML::Valid::False, MSAPI::HTML::Type::Body, MSAPI::HTML::Valid::True));
	RETURN_IF_FALSE(
		checkTag(14, 98, 104, 1, MSAPI::HTML::Valid::False, MSAPI::HTML::Type::Html, MSAPI::HTML::Valid::True));

	const std::string_view reallyHtml{
		"<!DOCTYPE html><html><head><!-- ALL META --><meta charset=\"utf-8\"/><meta name='robots' content=\"index, "
		"follow\" /></head><body data-trigger-id=\"body\">	<script>	   ym(80028238, \"init\", {	        "
		"clickmap:true,	        trackLinks:true,	        accurateTrackBounce:true,	        webvisor:true	   "
		"});	</script>	<!-- /Yandex.Metrika counter <div><ul></ul></div> -->	<header data-scrolled=\"header\" "
		"data-scrolled-type=\"sticker\">		<div class=\"indicator\"><div data-scrolled-indicator></div></div>	"
		"	<div class=\"width_main\">			<div class=\"menu_trigger\" data-trigger-toggle=\"menu, "
		"body\"><span></span></div>			<nav class=\"navigation\" data-trigger-id=\"menu\">				<div "
		"class=\"menu_close\" data-trigger-remove='menu, body'></div>				<ul class=\"nav_main\">			"
		"		<li><a href=\"\" title=\"\">JS решения</a></li>				</ul>				<ul "
		"class=\"nav_sub\">					<li><a href=\"/sliders\" title=\"Слайдеры\">Слайдеры</a></li>		"
		"			<li><a href=\"/triggers\" title=\"Реакция на клик\">Реакция на клик</a></li>				"
		"</ul>			</nav>		</div>	</header>	<main>		<section class=\"section_1\">		"
		"</section>	</main>	<footer>		<div class=\"width_main\">			<a class=\"witech\" "
		"href=\"https://witech.su\" title=\"Технологический партнер\" target='_blank'><img "
		"src=\"https://witech.su/assets/components/images/system/witech-isolated-04.png\" alt=\"witech\" "
		"title=\"Технологический партнер\"></a>		</div>	</footer>	<script "
		"src=\"js/trigger_ML_v2.js\"></script></body></html>"
	};

	MSAPI::HTML page(reallyHtml);
	LOG_DEBUG(page.ToString());

	RETURN_IF_FALSE(t.Assert(page.MaxDepth(), 8, "HTML Depth (complex)"));
	RETURN_IF_FALSE(t.Assert(page.BodySize(), reallyHtml.size(), "HTML Size (complex)"));
	RETURN_IF_FALSE(t.Assert(page.TagsSize(), 58, "HTML tags size (complex)"));
	RETURN_IF_FALSE(t.Assert(page.GetTag(), page.GetTag(page.TagsSize() + 1), "HTML get default tag (complex)"));

	return true;
};

}; // namespace UnitTest

}; // namespace MSAPI

#endif // MSAPI_UNIT_TEST_HTML_INL