#include "includes.h"

namespace render {
	Font menu;;
	Font menu_shade;;
	Font esp;;
	Font esp_small;;
	Font hud;;
	Font cs;;
	Font indicator;;
	Font icons1;;
	Font segoe;;

	Font gw;;
	Font csgh;;
	Font logevent;;
	Font bold;;
	Font norm;;
}

void render::init() {
	menu = Font(XOR("Tahoma"), 12, FW_NORMAL, FONTFLAG_NONE);
	menu_shade = Font(XOR("Tahoma"), 12, FW_NORMAL, FONTFLAG_DROPSHADOW);
	esp = Font(XOR("Verdana"), 12, FW_NORMAL, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
	esp_small = Font(XOR("Small Fonts"), 9, FW_NORMAL, FONTFLAG_OUTLINE);
	hud = Font(XOR("Tahoma"), 16, FW_NORMAL, FONTFLAG_ANTIALIAS);
	cs = Font(XOR("undefeated"), 11, FW_MEDIUM, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
	indicator = Font(XOR("Verdana"), 26, FW_BOLD, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
	icons1 = Font(XOR("undefeated"), 20, FW_MEDIUM, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
	segoe = Font(XOR("Verdana"), 12, FW_NORMAL, FONTFLAG_OUTLINE);

	logevent = Font(XOR("Lucida Console"), 10, FW_DONTCARE, FONTFLAG_DROPSHADOW);
	bold = Font(XOR("Tahoma Bold"), 12, FW_MEDIUM, FONTFLAG_DROPSHADOW);
	norm = Font(XOR("Tahoma"), 12, FW_NORMAL, FONTFLAG_DROPSHADOW);
	gw = Font(XOR("undefeated"), 24, FW_MEDIUM, FONTFLAG_ANTIALIAS);
	csgh = Font(XOR("undefeated"), 25, FW_MEDIUM, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
}

bool render::WorldToScreen(const vec3_t& world, vec2_t& screen) {
	float w;

	const VMatrix& matrix = g_csgo.m_engine->WorldToScreenMatrix();

	// check if it's in view first.
	// note - dex; w is below 0 when world position is around -90 / +90 from the player's camera on the y axis.
	w = matrix[3][0] * world.x + matrix[3][1] * world.y + matrix[3][2] * world.z + matrix[3][3];
	if (w < 0.001f)
		return false;

	// calculate x and y.
	screen.x = matrix[0][0] * world.x + matrix[0][1] * world.y + matrix[0][2] * world.z + matrix[0][3];
	screen.y = matrix[1][0] * world.x + matrix[1][1] * world.y + matrix[1][2] * world.z + matrix[1][3];

	screen /= w;

	// calculate screen position.
	screen.x = (g_cl.m_width / 2) + (screen.x * g_cl.m_width) / 2;
	screen.y = (g_cl.m_height / 2) - (screen.y * g_cl.m_height) / 2;

	return true;
}

void render::line(vec2_t v0, vec2_t v1, Color color) {
	g_csgo.m_surface->DrawSetColor(color);
	g_csgo.m_surface->DrawLine(v0.x, v0.y, v1.x, v1.y);
}

void render::gradient(int x, int y, int w, int h, Color color1, Color color2) {
	g_csgo.m_surface->DrawSetColor(color1);
	g_csgo.m_surface->DrawFilledRectFade(x, y, x + w, y + h, color1.a(), 0, false);

	g_csgo.m_surface->DrawSetColor(color2);
	g_csgo.m_surface->DrawFilledRectFade(x, y, x + w, y + h, 0, color2.a(), false);
}

void render::line(int x0, int y0, int x1, int y1, Color color) {
	g_csgo.m_surface->DrawSetColor(color);
	g_csgo.m_surface->DrawLine(x0, y0, x1, y1);
}

void render::rect(int x, int y, int w, int h, Color color) {
	g_csgo.m_surface->DrawSetColor(color);
	g_csgo.m_surface->DrawOutlinedRect(x, y, x + w, y + h);
}

void render::rect_filled(int x, int y, int w, int h, Color color) {
	g_csgo.m_surface->DrawSetColor(color);
	g_csgo.m_surface->DrawFilledRect(x, y, x + w, y + h);
}

void render::rect_filled_fade(int x, int y, int w, int h, Color color, int a1, int a2) {
	g_csgo.m_surface->DrawSetColor(color);
	g_csgo.m_surface->DrawFilledRectFade(x, y, x + w, y + h, a1, a2, false);
}

void render::rect_outlined(int x, int y, int w, int h, Color color, Color color2) {
	rect(x, y, w, h, color);
	rect(x - 1, y - 1, w + 2, h + 2, color2);
	rect(x + 1, y + 1, w - 2, h - 2, color2);
}

void render::circle(int x, int y, int radius, int segments, Color color) {
	static int texture = g_csgo.m_surface->CreateNewTextureID(true);

	g_csgo.m_surface->DrawSetTextureRGBA(texture, &colors::white, 1, 1);
	g_csgo.m_surface->DrawSetColor(color);
	g_csgo.m_surface->DrawSetTexture(texture);

	std::vector< Vertex > vertices{};

	float step = math::pi_2 / segments;
	for (float i{ 0.f }; i < math::pi_2; i += step)
		vertices.emplace_back(vec2_t{ x + (radius * std::cos(i)), y + (radius * std::sin(i)) });

	g_csgo.m_surface->DrawTexturedPolygon(vertices.size(), vertices.data());
}
void render::gradient(int x, int y, int w, int h, Color color1, Color color2, bool ciocan) {
	g_csgo.m_surface->DrawSetColor(color1);
	g_csgo.m_surface->DrawFilledRectFade(x, y, x + w, y + h, color1.a(), 0, ciocan);

	g_csgo.m_surface->DrawSetColor(color2);
	g_csgo.m_surface->DrawFilledRectFade(x, y, x + w, y + h, 0, color2.a(), ciocan);
}

void render::sphere(vec3_t origin, float radius, float angle, float scale, Color color) {
	std::vector< Vertex > vertices{};

	// compute angle step for input radius and precision.
	float step = (1.f / radius) + math::deg_to_rad(angle);

	for (float lat{}; lat < (math::pi * scale); lat += step) {
		// reset.
		vertices.clear();

		for (float lon{}; lon < math::pi_2; lon += step) {
			vec3_t point{
				origin.x + (radius * std::sin(lat) * std::cos(lon)),
				origin.y + (radius * std::sin(lat) * std::sin(lon)),
				origin.z + (radius * std::cos(lat))
			};

			vec2_t screen;
			if (WorldToScreen(point, screen))
				vertices.emplace_back(screen);
		}

		if (vertices.empty())
			continue;

		g_csgo.m_surface->DrawSetColor(color);
		g_csgo.m_surface->DrawTexturedPolyLine(vertices.size(), vertices.data());
	}
}

Vertex render::RotateVertex(const vec2_t& p, const Vertex& v, float angle) {
	// convert theta angle to sine and cosine representations.
	float c = std::cos(math::deg_to_rad(angle));
	float s = std::sin(math::deg_to_rad(angle));

	return {
		p.x + (v.m_pos.x - p.x) * c - (v.m_pos.y - p.y) * s,
		p.y + (v.m_pos.x - p.x) * s + (v.m_pos.y - p.y) * c
	};
}

void render::Font::string(int x, int y, Color color, const std::string& text, StringFlags_t flags /*= render::ALIGN_LEFT */) {
	wstring(x, y, color, util::MultiByteToWide(text), flags);
}

void render::Font::string(int x, int y, Color color, const std::stringstream& text, StringFlags_t flags /*= render::ALIGN_LEFT */) {
	wstring(x, y, color, util::MultiByteToWide(text.str()), flags);
}

void render::Font::wstring(int x, int y, Color color, const std::wstring& text, StringFlags_t flags /*= render::ALIGN_LEFT */) {
	int w, h;

	g_csgo.m_surface->GetTextSize(m_handle, text.c_str(), w, h);
	g_csgo.m_surface->DrawSetTextFont(m_handle);
	g_csgo.m_surface->DrawSetTextColor(color);

	if (flags & ALIGN_RIGHT)
		x -= w;
	if (flags & render::ALIGN_CENTER)
		x -= w / 2;

	g_csgo.m_surface->DrawSetTextPos(x, y);
	g_csgo.m_surface->DrawPrintText(text.c_str(), (int)text.size());
}

render::FontSize_t render::Font::size(const std::string& text) {
	return wsize(util::MultiByteToWide(text));
}

render::FontSize_t render::Font::wsize(const std::wstring& text) {
	FontSize_t res;
	g_csgo.m_surface->GetTextSize(m_handle, text.data(), res.m_width, res.m_height);
	return res;
}

void render::round_rect(int x, int y, int w, int h, int r, Color color) {
	Vertex round[64];

	for (int i = 0; i < 4; i++) {
		int _x = x + ((i < 2) ? (w - r) : r);
		int _y = y + ((i % 3) ? (h - r) : r);

		float a = 90.f * i;

		for (int j = 0; j < 16; j++) {
			float _a = math::deg_to_rad(a + j * 6.f);

			round[(i * 16) + j] = Vertex(vec2_t(_x + r * sin(_a), _y - r * cos(_a)));
		}
	}

	g_csgo.m_surface->DrawSetColor(color);
	g_csgo.m_surface->DrawTexturedPolygon(64, round);
}

/*
void render::circle_outline( int x, int y, int radius, int segments, Color color ) {
	static int texture = g_csgo.m_surface->CreateNewTextureID( true );

	g_csgo.m_surface->DrawSetTextureRGBA( texture, &colors::white, 1, 1 );
	g_csgo.m_surface->DrawSetColor( color );
	g_csgo.m_surface->DrawSetTexture( texture );

	std::vector< Vertex > vertices{};

	float step = math::pi_2 / segments;
	for ( float i{ 0.f }; i < math::pi_2; i += step )
		vertices.emplace_back( vec2_t{ x + ( radius * std::cos( i ) ), y + ( radius * std::sin( i ) ) } );

	g_csgo.m_surface->DrawTexturedPolyLine( vertices.size( ), vertices.data( ) );
}
*/

void render::TexturedPolygon(int n, std::vector< Vertex > vertice, Color color) {
	static int texture_id = g_csgo.m_surface->CreateNewTextureID(true); // 
	static unsigned char buf[4] = { 255, 255, 255, 255 };
	g_csgo.m_surface->DrawSetTextureRGBA(texture_id, buf, 1, 1); //
	g_csgo.m_surface->DrawSetColor(color); //
	g_csgo.m_surface->DrawSetTexture(texture_id); //
	g_csgo.m_surface->DrawTexturedPolygon(n, vertice.data()); //
}

void render::DrawFilledCircle(int x, int y, int radius, int segments, Color color) {
	std::vector< Vertex > vertices;

	float step = math::pi * 2.0f / segments;
	for (float a = 0; a < (math::pi * 2.0f); a += step)
		vertices.emplace_back(radius * cosf(a) + x, radius * sinf(a) + y);

	TexturedPolygon(vertices.size(), vertices, color);
}

void render::DrawLine(int x0, int y0, int x1, int y1, Color col, bool shadow)
{
	g_csgo.m_surface->DrawSetColor(col);
	g_csgo.m_surface->DrawLine(x0, y0, x1, y1);
}

bool render::WorldToScreen2(const vec3_t& world, vec3_t& screen) {
	float w;

	const VMatrix& matrix = g_csgo.m_engine->WorldToScreenMatrix();

	// check if it's in view first.
	// note - dex; w is below 0 when world position is around -90 / +90 from the player's camera on the y axis.
	w = matrix[3][0] * world.x + matrix[3][1] * world.y + matrix[3][2] * world.z + matrix[3][3];
	if (w < 0.001f)
		return false;

	// calculate x and y.
	screen.x = matrix[0][0] * world.x + matrix[0][1] * world.y + matrix[0][2] * world.z + matrix[0][3];
	screen.y = matrix[1][0] * world.x + matrix[1][1] * world.y + matrix[1][2] * world.z + matrix[1][3];

	screen /= w;

	// calculate screen position.
	screen.x = (g_cl.m_width / 2) + (screen.x * g_cl.m_width) / 2;
	screen.y = (g_cl.m_height / 2) - (screen.y * g_cl.m_height) / 2;

	return true;
}

#define ZERO vec3_t(0.0f, 0.0f, 0.0f)
void render::Draw3DFilledCircle(const vec3_t& origin, float radius, Color color) // OUTLINE 
{
	auto prevScreenPos = ZERO; //-V656
	auto step = math::pi * 2.0f / 72.0f;

	auto screenPos = ZERO;
	auto screen = ZERO;

	if (!render::WorldToScreen2(origin, screen))
		return;

	for (auto rotation = 0.0f; rotation <= math::pi * 2.0f; rotation += step) //-V1034
	{
		vec3_t pos(radius * cos(rotation) + origin.x, radius * sin(rotation) + origin.y, origin.z);

		if (render::WorldToScreen2(pos, screenPos))
		{
			if (!prevScreenPos.is_zero() && prevScreenPos.valid() && screenPos.valid() && prevScreenPos != screenPos)
			{
				render::DrawLine(prevScreenPos.x, prevScreenPos.y, screenPos.x, screenPos.y, color, true);
			}

			prevScreenPos = screenPos;
		}
	}
}

void render::Font::semi_filled_text(int x, int y, Color color, const std::string& text, StringFlags_t flags, float factor)
{
	auto indicator_size = wsize(util::MultiByteToWide(text));
	auto position = vec2_t(x, y);

	wstring(x, y, Color(30, 30, 30, 200), util::MultiByteToWide(text), flags);
	*(bool*)((DWORD)g_csgo.m_surface + 0x280) = true;
	int x1, y1, x2, y2;
	g_csgo.m_surface->get_drawing_area(x1, y1, x2, y2);
	g_csgo.m_surface->limit_drawing_area(position.x, position.y, int(indicator_size.m_width * factor), (int)indicator_size.m_height);

	wstring(x, y, color, util::MultiByteToWide(text), flags);

	g_csgo.m_surface->limit_drawing_area(x1, y1, x2, y2);
	*(bool*)((DWORD)g_csgo.m_surface - +0x280) = false;
}

void render::Font::semi_filled_text_v(int x, int y, Color color, const std::string& text, StringFlags_t flags, float factor)
{
	auto indicator_size = wsize(util::MultiByteToWide(text));
	auto position = vec2_t(x, y);

	wstring(x, y, Color(30, 30, 30, 200), util::MultiByteToWide(text), flags);
	*(bool*)((DWORD)g_csgo.m_surface + 0x280) = true;
	int x1, y1, x2, y2;
	g_csgo.m_surface->get_drawing_area(x1, y1, x2, y2);
	g_csgo.m_surface->limit_drawing_area(position.x, position.y, int(indicator_size.m_width), (int)indicator_size.m_height * factor);

	wstring(x, y, color, util::MultiByteToWide(text), flags);

	g_csgo.m_surface->limit_drawing_area(x1, y1, x2, y2);
	*(bool*)((DWORD)g_csgo.m_surface - +0x280) = false;
}

void render::Triangle(vec2_t point_one, vec2_t point_two, vec2_t point_three, Color color) {

	Vertex verts[3] = {
		Vertex(point_one),
		Vertex(point_two),
		Vertex(point_three)
	};

	static int texture = g_csgo.m_surface->CreateNewTextureID(true);
	unsigned char buffer[4] = { 255, 255, 255, 255 };

	g_csgo.m_surface->DrawSetTextureRGBA(texture, buffer, 1, 1);
	g_csgo.m_surface->DrawSetColor(color);
	g_csgo.m_surface->DrawSetTexture(texture);

	g_csgo.m_surface->DrawTexturedPolygon(3, verts);
}

void render::circle_outline(int x, int y, int radius, int segments, Color color) {
	g_csgo.m_surface->DrawSetColor(color);
	g_csgo.m_surface->DrawOutlinedCircle(x, y, radius, segments);
}

void render::WorldCircleOutline(vec3_t origin, float radius, float angle, Color color) {
	std::vector< Vertex > vertices{};

	float step = (1.f / radius) + math::deg_to_rad(angle);

	float lat = 1.f;
	vertices.clear();

	for (float lon{}; lon < math::pi_2; lon += step) {
		vec3_t point{
			origin.x + (radius * std::sin(lat) * std::cos(lon)),
			origin.y + (radius * std::sin(lat) * std::sin(lon)),
			origin.z + (radius * std::cos(lat) - (radius / 2))
		};

		vec2_t screen;
		if (WorldToScreen(point, screen))
			vertices.emplace_back(screen);
	}
	static int texture = g_csgo.m_surface->CreateNewTextureID(false);

	g_csgo.m_surface->DrawSetTextureRGBA(texture, &colors::white, 1, 1);
	g_csgo.m_surface->DrawSetColor(color);

	//g_csgo.m_surface->DrawSetTexture(texture);
	//g_csgo.m_surface->DrawTexturedPolygon(vertices.size(), vertices.data());

	g_csgo.m_surface->DrawTexturedPolyLine(vertices.size(), vertices.data());
}

void render::textured_polygon(int n, std::vector<Vertex> vertice, Color color)
{
	auto g_VGuiSurface = g_csgo.m_surface;
	static int texture_id = g_VGuiSurface->CreateNewTextureID(true); //
	static unsigned char buf[4] = { 255, 255, 255, 255 };
	g_VGuiSurface->DrawSetTextureRGBA(texture_id, buf, 1, 1); //
	g_VGuiSurface->DrawSetColor(color); //
	g_VGuiSurface->DrawSetTexture(texture_id); //
	g_VGuiSurface->DrawTexturedPolygon(n, vertice.data()); //
}

void render::textured_polyline(int n, std::vector<Vertex> vertice, Color color)
{
	auto g_VGuiSurface = g_csgo.m_surface;
	static int texture_id = g_VGuiSurface->CreateNewTextureID(true); //
	static unsigned char buf[4] = { 255, 255, 255, 255 };
	g_VGuiSurface->DrawSetTextureRGBA(texture_id, buf, 1, 1); //
	g_VGuiSurface->DrawSetColor(color); //
	g_VGuiSurface->DrawSetTexture(texture_id); //
	g_VGuiSurface->DrawTexturedPolyLine(n, vertice.data()); //
}

void render::draw_dynamic_filed_circle(const vec3_t& origin, float radius, Color color, Color color_fill, float points)
{
	auto screen = ZERO;

	if (!render::world_to_screen_3d(origin, screen))
		return;

	static float Step = (M_PI * 2.0f) / (points);
	std::vector<Vertex> point;
	for (float lat = 0.f; lat <= M_PI * 2.0f; lat += Step)
	{
		const auto& point3d = vec3_t(sin(lat), cos(lat), 0.f) * radius;
		vec2_t point2d;
		if (render::WorldToScreen(origin + point3d, point2d))
			point.push_back(vec2_t(point2d.x, point2d.y));
	}
	textured_polygon(point.size(), point, color_fill);
	textured_polyline(point.size(), point, color);
}

bool render::world_to_screen_3d(const vec3_t& world, vec3_t& screen) {
	float w;

	const VMatrix& matrix = g_csgo.m_engine->WorldToScreenMatrix();

	// check if it's in view first.
	// note - dex; w is below 0 when world position is around -90 / +90 from the player's camera on the y axis.
	w = matrix[3][0] * world.x + matrix[3][1] * world.y + matrix[3][2] * world.z + matrix[3][3];
	if (w < 0.001f)
		return false;

	// calculate x and y.
	screen.x = matrix[0][0] * world.x + matrix[0][1] * world.y + matrix[0][2] * world.z + matrix[0][3];
	screen.y = matrix[1][0] * world.x + matrix[1][1] * world.y + matrix[1][2] * world.z + matrix[1][3];

	screen /= w;

	// calculate screen position.
	screen.x = (g_cl.m_width / 2) + (screen.x * g_cl.m_width) / 2;
	screen.y = (g_cl.m_height / 2) - (screen.y * g_cl.m_height) / 2;

	return true;
}
