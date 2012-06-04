#ifndef TRACK_H
#define TRACK_H

#include <account/user.h>

class Track{

    public:

        Track() { id = 0; }
        Track(int tid);

        int id;
        std::string title;
        User artist;
        std::string date;
        bool visible;

        std::string url() const;

        void fill(Dict *d) const;

        operator bool() const { return id > 0; }
        bool operator==(const Track &t) { return t.id == id; }

};

#endif //TRACK_H
