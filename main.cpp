#include <wcore/log.hpp>
#include <wplatform/clipboard.hpp>
#include <wplatform/cursor.hpp>
#include <wplatform/events.hpp>
#include <wplatform/input.hpp>
#include <wplatform/joystick.hpp>
#include <wplatform/monitor.hpp>
#include <wplatform/platform.hpp>
#include <wplatform/version.hpp>
#include <wplatform/window.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

using namespace wplatform;

namespace
{
	constexpr std::size_t IconSize{ 16 };

	void log_monitors()
	{
		WCORE_INFO("monitors: {}", monitor::count());

		for (const auto& display : monitor::all())
		{
			const auto mode{ display.current_mode() };
			const auto area{ display.work_area() };

			WCORE_INFO("  {}: {}x{} at ({}, {}), workspace {}x{}",
				display.name(),
				mode ? mode->size.width : 0u,
				mode ? mode->size.height : 0u,
				display.position().x,
				display.position().y,
				area.size.width,
				area.size.height);
		}
	}

	void log_joysticks()
	{
		WCORE_INFO("joysticks: {}", joystick::connected_count());

		for (const auto id : joystick::connected())
		{
			const auto deviceName{ joystick::name(id) };
			const auto state{ joystick::state(id) };

			WCORE_INFO("  joy {}: {}{}",
				static_cast<i32>(id) + 1,
				deviceName ? *deviceName : std::string{ "<unknown>" },
				joystick::is_gamepad(id) ? " (gamepad)" : "");

			if (state && !state->hats.empty()
				&& (state->hat(0) & joystick::HatUp) != 0)
			{
				WCORE_INFO("    hat up");
			}
		}
	}

	window::Icon make_icon()
	{
		window::Icon icon;
		icon.size = { static_cast<u32>(IconSize), static_cast<u32>(IconSize) };
		icon.pixels.assign(IconSize * IconSize * 4, 0);

		for (std::size_t y{ 0 }; y < IconSize; ++y)
		{
			for (std::size_t x{ 0 }; x < IconSize; ++x)
			{
				if (x == 0 || y == 0 || x == IconSize - 1 || y == IconSize - 1)
				{
					const auto offset{ (y * IconSize + x) * 4 };
					icon.pixels[offset + 0] = 240;
					icon.pixels[offset + 1] = 240;
					icon.pixels[offset + 2] = 255;
					icon.pixels[offset + 3] = 255;
				}
			}
		}

		return icon;
	}

	cursor::Image make_cursor_image()
	{
		cursor::Image image;
		image.size = { static_cast<u32>(IconSize), static_cast<u32>(IconSize) };
		image.pixels.assign(IconSize * IconSize * 4, 0);

		for (std::size_t i{ 0 }; i < IconSize; ++i)
		{
			const auto vertical{ (i * IconSize + IconSize / 2) * 4 };
			const auto horizontal{ ((IconSize / 2) * IconSize + i) * 4 };

			image.pixels[vertical + 0] = 255;
			image.pixels[vertical + 3] = 255;
			image.pixels[horizontal + 0] = 255;
			image.pixels[horizontal + 3] = 255;
		}

		return image;
	}

	void add_listeners(window::Window& target)
	{
		auto* dispatcher{ target.dispatcher() };

		dispatcher->add_listener(events::Type::resize,
			[](const events::Event& event)
			{
				const auto* resized{ event.get<events::Resized>() };
				if (resized != nullptr)
				{
					WCORE_INFO("event: resize {}x{}",
						resized->size.width, resized->size.height);
				}
			});

		dispatcher->add_listener(events::Type::focus,
			[](const events::Event& event)
			{
				const auto* focus{ event.get<events::FocusChanged>() };
				if (focus != nullptr)
					WCORE_INFO("event: focus {}", focus->focused);
			});

		dispatcher->add_listener(events::Type::char_input,
			[](const events::Event& event)
			{
				const auto* character{ event.get<events::CharEvent>() };
				if (character != nullptr)
					WCORE_INFO("event: char U+{:04X}", character->codepoint);
			});

		dispatcher->add_listener(events::Type::scroll,
			[](const events::Event& event)
			{
				const auto* scrolled{ event.get<events::Scrolled>() };
				if (scrolled != nullptr)
					WCORE_INFO("event: scroll ({}, {})", scrolled->x, scrolled->y);
			});

		dispatcher->add_listener(events::Type::drop,
			[](const events::Event& event)
			{
				const auto* dropped{ event.get<events::FileDropped>() };
				if (dropped != nullptr)
				{
					for (const auto& path : dropped->paths)
						WCORE_INFO("event: drop {}", path);
				}
			});

		dispatcher->add_listener(events::Type::monitor_connected,
			[](const events::Event& event)
			{
				const auto* connected{ event.get<events::MonitorConnected>() };
				if (connected != nullptr)
				{
					WCORE_INFO("event: monitor connected {}", connected->handle.name());
				}
			});

		dispatcher->add_listener(events::Type::monitor_disconnected,
			[](const events::Event& event)
			{
				const auto* disconnected{ event.get<events::MonitorDisconnected>() };
				if (disconnected != nullptr)
				{
					WCORE_INFO("event: monitor disconnected {}", disconnected->name);
				}
			});

		dispatcher->add_listener(events::Type::joystick_connected,
			[](const events::Event& event)
			{
				const auto* connected{ event.get<events::JoystickConnected>() };
				if (connected != nullptr)
				{
					WCORE_INFO("event: joystick {} connected",
						static_cast<i32>(connected->id) + 1);
				}
			});

		dispatcher->add_listener(events::Type::joystick_disconnected,
			[](const events::Event& event)
			{
				const auto* disconnected{ event.get<events::JoystickDisconnected>() };
				if (disconnected != nullptr)
				{
					WCORE_INFO("event: joystick {} disconnected",
						static_cast<i32>(disconnected->id) + 1);
				}
			});
	}

	void poll_joysticks(std::unordered_set<i32>& previousA)
	{
		std::unordered_set<i32> currentA;

		for (const auto id : joystick::connected())
		{
			const auto state{ joystick::gamepad_state(id) };
			if (!state || !state->down(joystick::GamepadButton::a))
				continue;

			const auto value{ static_cast<i32>(id) };
			currentA.insert(value);

			if (previousA.find(value) == previousA.end())
			{
				const auto gamepadName{ joystick::gamepad_name(id) };
				WCORE_INFO("gamepad A pressed: joy {} ({})", value + 1,
					gamepadName ? *gamepadName : std::string{ "<unknown>" });
			}
		}

		previousA = std::move(currentA);
	}
} // namespace

int main()
{
	wcore::log::init();

	WCORE_INFO("wplatform example: v{}", to_string(CurrentVersion));

	platform::set_error_handler([](const platform::Error& error)
	{
		std::cerr << "GLFW " << error.code << ": " << error.message << '\n';
	});

	platform::Config platformConfig;
	platformConfig.joystickHatsAsButtons = false;

	if (!platform::init(platformConfig))
	{
		WCORE_ERROR("Failed to initialize the platform");
		return 1;
	}

	WCORE_INFO("Backend: {}", platform::backend_name());
	WCORE_INFO("GLFW: v{}", to_string(platform::glfw_version()));
	WCORE_INFO("Raw mouse motion: {}", cursor::raw_motion_supported());

	log_monitors();
	log_joysticks();

	{
		window::Config windowConfig;
		windowConfig.title = "wplatform test app";
		windowConfig.size = { 960, 540 };
		windowConfig.resizable = true;
		windowConfig.maximized = false;
		windowConfig.floating = false;
		windowConfig.autoIconify = true;
		windowConfig.mousePassthrough = false;
		windowConfig.transparentFramebuffer = false;
		windowConfig.sizeLimits.minWidth = 320;
		windowConfig.sizeLimits.minHeight = 240;

		auto window{ window::Window::create(windowConfig) };
		if (!window)
		{
			WCORE_ERROR("Failed to create the window");
			platform::shutdown();
			return 1;
		}

		add_listeners(*window);

		const auto icon{ make_icon() };
		window->set_icon(icon);

		const auto framebuffer{ window->framebuffer_size() };
		const auto scale{ window->content_scale() };
		WCORE_INFO("window: framebuffer {}x{}, scale ({}, {})",
			framebuffer.width, framebuffer.height, scale.x, scale.y);
		WCORE_INFO("attributes: floating={}, auto_iconify={}, mouse_passthrough={}, transparent={}",
			window->floating(), window->auto_iconify(), window->mouse_passthrough(),
			window->transparent_framebuffer());
		WCORE_INFO("controls: Esc close, C copy, V paste, H cursor mode, R raw motion, "
			"O opacity, T window flags, A attention, M fullscreen, G aspect, "
			"N clear limits, S maximize, I icons, U visibility, J rescan");

		const auto standardCursor{ cursor::Cursor::create(cursor::Shape::arrow) };
		const auto image{ make_cursor_image() };
		const auto customCursor{ cursor::Cursor::create(image,{ 8, 8 }) };

		if (customCursor && cursor::set(*window, *customCursor))
			WCORE_INFO("Using the custom image cursor");
		else if (standardCursor)
			cursor::set(*window, *standardCursor);

		bool fullscreen{ false };
		std::unordered_set<i32> previousA;

		while (!window->should_close())
		{
			events::poll();

			auto* input{ window->input() };

			if (input->key_pressed(events::Key::escape))
				window->request_close();

			if (input->key_pressed(events::Key::h))
			{
				const auto next{ cursor::mode(*window) == cursor::Mode::normal
					? cursor::Mode::hidden
					: cursor::Mode::normal };
				cursor::set_mode(*window, next);
			}

			if (input->key_pressed(events::Key::r) && cursor::raw_motion_supported())
			{
				const bool enabled{ !cursor::raw_motion(*window) };
				cursor::set_raw_motion(*window, enabled);
				WCORE_INFO("Raw mouse motion: {}", enabled);
			}

			if (input->key_pressed(events::Key::c))
				clipboard::set(*window, "Copied from wplatform");

			if (input->key_pressed(events::Key::v))
			{
				const auto text{ clipboard::get(*window) };
				WCORE_INFO("Clipboard: {}", text ? *text : std::string{ "<empty>" });
			}

			if (input->key_pressed(events::Key::o))
			{
				const auto next{ window->opacity() > 0.9f ? 0.5f : 1.0f };
				window->set_opacity(next);
			}

			if (input->key_pressed(events::Key::t))
			{
				window->set_floating(!window->floating());
				window->set_auto_iconify(!window->auto_iconify());
				window->set_mouse_passthrough(!window->mouse_passthrough());
			}

			if (input->key_pressed(events::Key::g))
				window->set_aspect_ratio(16, 9);

			if (input->key_pressed(events::Key::n))
			{
				window->clear_aspect_ratio();
				window->clear_size_limits();
			}

			if (input->key_pressed(events::Key::s))
			{
				if (window->maximized() || window->iconified())
					window->restore();
				else
					window->maximize();
			}

			if (input->key_pressed(events::Key::i))
			{
				const std::array<window::Icon, 2> icons{ icon, icon };
				window->set_icons(icons);
			}

			if (input->key_pressed(events::Key::u))
			{
				if (window->visible())
					window->hide();
				else
					window->show();
			}

			if (input->key_pressed(events::Key::a))
				window->request_attention();

			if (input->key_pressed(events::Key::m))
			{
				if (!fullscreen)
				{
					const auto primary{ monitor::primary() };
					const auto videoMode{ primary ? primary->current_mode()
						: std::nullopt };

					if (primary && videoMode)
						fullscreen = window->set_fullscreen(*primary, *videoMode);
				}
				else
				{
					fullscreen = !window->set_windowed({ 100, 100 }, { 960, 540 });
				}
			}

			if (input->key_pressed(events::Key::j))
			{
				log_monitors();
				log_joysticks();
			}

			const auto cursorPosition{ input->cursor() };
			if (input->mouse_pressed(events::MouseButton::left))
			{
				WCORE_INFO("click at ({}, {}), hovered={}",
					cursorPosition.x, cursorPosition.y, window->hovered());
			}

			poll_joysticks(previousA);
			input->new_frame();
		}

		WCORE_INFO("Window closed");
	}

	platform::shutdown();
	return 0;
}