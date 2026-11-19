#include <wcore/log.hpp>
#include <wplatform/platform.hpp>
#include <wplatform/version.hpp>
#include <wplatform/window.hpp>

#include <GLFW/glfw3.h>

#include <iostream>

int main()
{
	using namespace wplatform;

	wcore::log::init();

	WCORE_INFO("wplatform: v{}", wplatform::to_string(wplatform::CurrentVersion));

	platform::set_error_handler([](
		const platform::Error& error)
	{
		std::cerr << "GLFW " << error.code <<
			": " << error.message << '\n';
	});

	if (!platform::init(platform::Config()))
	{
		std::cerr << "Failed to initialize platform" << ")\n";
		return 1;
	}

	WCORE_INFO("Backend: {}", platform::backend_name());
	WCORE_INFO("GLFW: v{}", wplatform::to_string(platform::glfw_version()));

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

		window->iconify();

		while (!window->should_close())
		{
			glfwPollEvents();

			if (glfwGetKey(window->native_handle(),
				GLFW_KEY_ESCAPE) == GLFW_PRESS)
			{
				window->request_close();
			}
		}
	}

	platform::shutdown();
}
