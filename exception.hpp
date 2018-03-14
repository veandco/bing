#pragma once

#include <exception>

using namespace std;

namespace Bing {

enum Error {
    HTTPError = 1,
    IOError = 2,
};

class Exception : public exception {
public:
    /**
     * Constructor
     *
     * \param errorCode Code number of the error
     */
    Exception(Error errorCode)
    {
        mErrorCode = errorCode;
    }

    /**
     * Description of the error
     */
    const char *what()
    {
        switch (mErrorCode) {
        case HTTPError: return "HTTP error";
        case IOError: return "IO error";
        default: return "unknown error";
        }
    }

    /**
     * Code number of the error
     */
    int code() const
    {
        return mErrorCode;
    }

private:
    Exception();

    int mErrorCode;
};

}
