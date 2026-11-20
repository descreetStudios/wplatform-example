#include <wcore/log.hpp>
#include <wplatform/events.hpp>
#include <wplatform/platform.hpp>
#include <wplatform/version.hpp>
#include <wplatform/window.hpp>

#include <iostream>

int main()
{
	using namespace wplatform;

	wcore::log::init();

	WCORE_INFO("wplatform: v{}", to_string(CurrentVersion));

	platform::set_error_handler([](const platform::Error& error)
	{
		std::cerr << "GLFW " << error.code << ": " << error.message << '\n';
	});

	if (!platform::init())
	{
		std::cerr << "Failed to initialize platform\n";
		return 1;
	}

	WCORE_INFO("Backend: {}", platform::backend_name());
	WCORE_INFO("GLFW: v{}", to_string(platform::glfw_version()));

	{
		window::Config windowConfig;
		windowConfig.title = "Hello wplatform";
		windowConfig.size = { 960, 540 };

		auto window = window::Window::create(windowConfig);
		if (!window)
		{
			WCORE_ERROR("Failed to create the window");
			platform::shutdown();
			return 1;
		}

		auto* dispatcher = window->dispatcher();

		dispatcher->add_listener(events::Type::key,
			[&window](const events::Event& event)
			{
				const auto* key = event.get<events::KeyEvent>();
				if (key == nullptr || key->action == events::Action::release)
					return;

				if (key->key == events::Key::escape)
				{
					WCORE_INFO("Escape pressed, closing");
					window->request_close();
				}
				else if (key->key == events::Key::f1)
				{
					window->set_title("wplatform events example");
				}
				else
				{
					WCORE_INFO("key {} pressed", static_cast<int>(key->key));
				}
			});

		dispatcher->add_listener(events::Type::resize,
			[](const events::Event& event)
			{
				if (const auto* resized = event.get<events::Resized>())
				{
					WCORE_INFO("window resized: {}x{}",
						resized->size.width, resized->size.height);
				}
			});

		dispatcher->add_listener(events::Type::focus,
			[](const events::Event& event)
			{
				if (const auto* focus = event.get<events::FocusChanged>())
					WCORE_INFO("window {}", focus->focused ? "focused" : "unfocused");
			});

		dispatcher->add_listener(events::Type::drop,
			[](const events::Event& event)
			{
				if (const auto* drop = event.get<events::FileDropped>())
				{
					for (const auto& path : drop->paths)
						WCORE_INFO("dropped: {}", path);
				}
			});

		dispatcher->add_listener(events::Type::close, [](const events::Event&)
		{
			WCORE_INFO("window close requested");
		});

		WCORE_INFO("Press Escape to quit, F1 to rename the window");

		while (!window->should_close())
			events::poll();
	}

	platform::shutdown();
}
