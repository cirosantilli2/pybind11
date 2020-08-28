/*
    pybind11/iostream.h -- Tools to assist with redirecting cout and cerr to Python

    Copyright (c) 2017 Henry F. Schreiner

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include "pybind11.h"

#include <streambuf>
#include <ostream>
#include <string>
#include <memory>
#include <iostream>

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)

// Buffer that writes to Python instead of C++
class pythonbuf : public std::streambuf {
private:
    using traits_type = std::streambuf::traits_type;

    const size_t buf_size;
    std::unique_ptr<char[]> d_buffer;
    object pywrite;
    object pyflush;

    int overflow(int c);

    int sync();

public:

    pythonbuf(object pyostream, size_t buffer_size = 1024);

    pythonbuf(pythonbuf&&) = default;

    /// Sync before destroy
    ~pythonbuf();
};

PYBIND11_NAMESPACE_END(detail)


/** \rst
    This a move-only guard that redirects output.

    .. code-block:: cpp

        #include <pybind11/iostream.h>

        ...

        {
            py::scoped_ostream_redirect output;
            std::cout << "Hello, World!"; // Python stdout
        } // <-- return std::cout to normal

    You can explicitly pass the c++ stream and the python object,
    for example to guard stderr instead.

    .. code-block:: cpp

        {
            py::scoped_ostream_redirect output{std::cerr, py::module::import("sys").attr("stderr")};
            std::cerr << "Hello, World!";
        }
 \endrst */
class scoped_ostream_redirect {
protected:
    std::streambuf *old;
    std::ostream &costream;
    detail::pythonbuf buffer;

public:
    scoped_ostream_redirect(
            std::ostream &costream = std::cout,
            object pyostream = module::import("sys").attr("stdout"));

    ~scoped_ostream_redirect();

    scoped_ostream_redirect(const scoped_ostream_redirect &) = delete;
    scoped_ostream_redirect(scoped_ostream_redirect &&other) = default;
    scoped_ostream_redirect &operator=(const scoped_ostream_redirect &) = delete;
    scoped_ostream_redirect &operator=(scoped_ostream_redirect &&) = delete;
};


/** \rst
    Like `scoped_ostream_redirect`, but redirects cerr by default. This class
    is provided primary to make ``py::call_guard`` easier to make.

    .. code-block:: cpp

     m.def("noisy_func", &noisy_func,
           py::call_guard<scoped_ostream_redirect,
                          scoped_estream_redirect>());

\endrst */
class scoped_estream_redirect : public scoped_ostream_redirect {
public:
    scoped_estream_redirect(
            std::ostream &costream = std::cerr,
            object pyostream = module::import("sys").attr("stderr"))
        : scoped_ostream_redirect(costream,pyostream) {}
};


PYBIND11_NAMESPACE_BEGIN(detail)

// Class to redirect output as a context manager. C++ backend.
class OstreamRedirect {
    bool do_stdout_;
    bool do_stderr_;
    std::unique_ptr<scoped_ostream_redirect> redirect_stdout;
    std::unique_ptr<scoped_estream_redirect> redirect_stderr;

public:
    OstreamRedirect(bool do_stdout = true, bool do_stderr = true)
        : do_stdout_(do_stdout), do_stderr_(do_stderr) {}

    void enter();

    void exit();
};

PYBIND11_NAMESPACE_END(detail)

/** \rst
    This is a helper function to add a C++ redirect context manager to Python
    instead of using a C++ guard. To use it, add the following to your binding code:

    .. code-block:: cpp

        #include <pybind11/iostream.h>

        ...

        py::add_ostream_redirect(m, "ostream_redirect");

    You now have a Python context manager that redirects your output:

    .. code-block:: python

        with m.ostream_redirect():
            m.print_to_cout_function()

    This manager can optionally be told which streams to operate on:

    .. code-block:: python

        with m.ostream_redirect(stdout=true, stderr=true):
            m.noisy_function_with_error_printing()

 \endrst */
class_<detail::OstreamRedirect> add_ostream_redirect(module m, std::string name = "ostream_redirect");

PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)

#if !defined(PYBIND11_DECLARATIONS_ONLY)
#include "iostream.cpp"
#endif
