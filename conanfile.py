from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class VulkanRendererRecipe(ConanFile):
    name = "vulkan-renderer"
    version = "1.2"

    # Optional metadata
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of hello package here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    exports_sources = "CMakeLists.txt", "include/VulkanRenderer/*", "source/*", "cmake/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()
        
    def requirements(self):
        self.requires("polymorph/1.1", transitive_headers=True)
        self.requires("simple-geometry/1.0", transitive_headers=True)
        self.requires("glm/1.0.1", transitive_headers=True)
        #self.requires("sdl/[~2.28]", transitive_headers=True)
        #self.requires("vulkan-headers/1.4.309.0", transitive_headers=True)


    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["vulkan-renderer"]
