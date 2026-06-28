from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout


class DynamicArrayConan(ConanFile):
    name = "dynamic_array"
    version = "1.0.0"

    # ------------------------------------------------------------------
    # 包元数据
    # ------------------------------------------------------------------
    package_type = "static-library"
    license = "GPL-3.0-or-later"
    author = "mtueih"
    description = (
        "A type-safe dynamic array library in C "
        "(create / insert / remove / sort / find / clone …)."
    )
    topics = ("dynamic-array", "c", "data-structure")
    url = "https://github.com/mtueih/dynamic_array"

    # ------------------------------------------------------------------
    # 配置维度
    # ------------------------------------------------------------------
    settings = "os", "compiler", "build_type", "arch"

    # ------------------------------------------------------------------
    # 随包分发的文件（不参与构建）
    # ------------------------------------------------------------------
    exports = "LICENSE"

    # ------------------------------------------------------------------
    # 打包时导出的源码（排除 tests/）
    # ------------------------------------------------------------------
    exports_sources = (
        "CMakeLists.txt",
        "src/*",
        "include/*",
        "cmake/*"
    )

    # ------------------------------------------------------------------
    # 布局
    # ------------------------------------------------------------------
    def layout(self):
        cmake_layout(self)

    # ------------------------------------------------------------------
    # 依赖声明
    # ------------------------------------------------------------------
    def requirements(self):
        # safe_calc 已通过 conan create . 存在于本机缓存中
        self.requires("safe_calc/0.1.0")

    # ------------------------------------------------------------------
    # 生成工具链 + 依赖 CMake 文件
    # ------------------------------------------------------------------
    def generate(self):
        tc = CMakeToolchain(self)
        # 打包时不构建测试，仅产出库
        tc.variables["BUILD_TESTING"] = "OFF"
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    # ------------------------------------------------------------------
    # 构建
    # ------------------------------------------------------------------
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # ------------------------------------------------------------------
    # 打包（复用 CMakeLists.txt 中已有的 install 规则）
    # ------------------------------------------------------------------
    def package(self):
        cmake = CMake(self)
        cmake.install()

    # ------------------------------------------------------------------
    # 消费者信息
    # ------------------------------------------------------------------
    def package_info(self):
        self.cpp_info.libs = ["dynamic_array"]
        self.cpp_info.set_property("cmake_target_name", "dynamic_array::dynamic_array")
        self.cpp_info.set_property("cmake_file_name", "dynamic_array")

