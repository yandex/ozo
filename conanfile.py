from conans import ConanFile, CMake
from conans.tools import load
import re


def get_version():
    try:
        content = load("CMakeLists.txt")
        version = re.search(r"^\s*project\(ozo\s+VERSION\s+([^\s)]+)", content, re.M).group(1)
        return version.strip()
    except Exception:
        return None


class Ozo(ConanFile):
    name = 'ozo'
    version = get_version()
    license = 'MIT'
    url = 'https://github.com/yandex/ozo'
    description = 'Conan package for yandex ozo'

    exports_sources = 'include/*', 'CMakeLists.txt', 'cmake/*', 'LICENCE', 'AUTHORS'

    generators = 'cmake_find_package'
    requires = ('boost/1.74.0', 'resource_pool/0.1.0', 'libpq/13.1')

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def package_id(self):
        self.info.header_only()

    def package_info(self):
        self.cpp_info.components["_ozo"].includedirs = ["include"]
        self.cpp_info.components["_ozo"].requires = ["boost::boost", "boost::system", "boost::thread", "boost::coroutine",
                                                     "resource_pool::resource_pool", # == elsid::resource_pool in cmake
                                                     "libpq::pq",                    # == PostgreSQL::PostgreSQL in cmake
                                                     ]
        self.cpp_info.components["_ozo"].defines = ["BOOST_COROUTINES_NO_DEPRECATION_WARNING", "BOOST_HANA_CONFIG_ENABLE_STRING_UDL", "BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT"]

        self.cpp_info.filenames["cmake_find_package"] = "ozo"
        self.cpp_info.filenames["cmake_find_package_multi"] = "ozo"
        self.cpp_info.names["cmake_find_package"] = "yandex"
        self.cpp_info.names["cmake_find_package_multi"] = "yandex"
        self.cpp_info.components["_ozo"].names["cmake_find_package"] = "ozo"
        self.cpp_info.components["_ozo"].names["cmake_find_package_multi"] = "ozo"
