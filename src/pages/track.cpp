#include "pages.h"
#include <account/session.h>
#include <core/cgi.h>
#include <core/db.h>
#include <log/log.h>
#include <misc/mail.h>
#include <playlist/playlist.h>
#include <social/event.h>
#include <social/follower.h>
#include <text/text.h>
#include <track/audio.h>
#include <track/art.h>
#include <track/extended.h>
#include <stat/push.h>
#include <youtube/youtube.h>

static std::string filter(const std::string &str){
    std::string buf;
    for(std::string::const_iterator i = str.begin(); i!=str.end(); i++){
        if(*i != '\n' && *i != '\r') buf += *i;
    }
    return buf;
}

void delete_track(Track &t){
    log("Deleting track: " + t.title + " (" + number(t.id) + ")");

    Art art(t.id);
    if(art) art.remove();
    Audio(&t).unlink();

    Playlist::removeTrack(t.id);
    DB::query("DELETE FROM events WHERE track_id = " + number(t.id));
    DB::query("DELETE FROM featured_tracks WHERE track_id = " + number(t.id));
    DB::query("DELETE FROM favorites WHERE type = 'track' AND ref = " + number(t.id));
    DB::query("DELETE FROM user_features WHERE type = 'track' AND ref = " + number(t.id));
    DB::query("DELETE FROM tracks WHERE id = " + number(t.id));
}

void publish_track(Track &t){
    DB::query("UPDATE tracks SET visible = 't', date = 'now' WHERE id = " + number(t.id));

    Event e;
    e.type = Event::Publish;
    e.source = t.artist;
    e.track = t;
    e.push();

    AccountList emails = Follower(t.artist.id).followers();
    std::string maildata =
        "From: EqBeats notification <notify@eqbeats.org>\n"
        "Message-ID: notify-t" + number(t.id) + "\n"
        "Subject: " + filter("EqBeats notification: " + t.artist.name + " - " + t.title) + "\n"
        "Precedence: bulk\n\n" +
        t.artist.name + " just published a new track : " + t.title + "\n"
        "Listen to it here : " + eqbeatsUrl() + t.url() + "\n\n"
        "You're receiving this email because you're following " + t.artist.name + " on EqBeats.\n"
        "If you don't want to receive these notifications anymore, go to " + eqbeatsUrl() + t.artist.url() + " and click \"Stop following\".";
    for(AccountList::const_iterator i = emails.begin(); i!=emails.end(); i++)
        sendMail(i->email.c_str(), maildata.c_str());
}

void Pages::track(Document *doc){

    std::string sub;
    int tid = route("track", path, sub);
    bool post = cgi.getEnvironment().getRequestMethod() == "POST";

    if(!tid) return;

    if(sub == ""){

        ExtendedTrack t(tid);
        if(!t) return;

        pushStat("trackView", t.artist.id, tid);

        doc->setHtml("html/track.tpl", t.title);
        doc->rootDict()->SetValueAndShowSection("TID", number(t.id), "HAS_OEMBED");
        t.fill(doc->dict());
        t.player(doc->dict(), true);
        Audio(&t).fill(doc->dict());

        doc->dict()->ShowSection(Youtube(t.artist.id) ? "HAS_YOUTUBE" : "NO_YOUTUBE");

        Dict *embed = doc->dict()->AddIncludeDictionary("EMBED_CODE");
        embed->SetFilename("html/embed-code.tpl");
        embed->SetIntValue("WIDTH", 150);
        t.Track::fill(embed);

        Dict *uploader = doc->dict()->AddIncludeDictionary("UPLOADER");
        uploader->SetFilename("html/uploader.tpl");
        uploader->SetValue("ACTION", t.url() + "/upload");

        int hits = t.artist.self() ? t.getHits() : t.hit();
        doc->dict()->SetValue("HIT_COUNT", number(hits));
        doc->rootDict()->ShowSection("REQUIRES_STATS_JS");

        Session::fill(doc->dict());
        EventList::track(t).fill(doc->dict(), "EVENTS", false);
        doc->dict()->ShowSection(Follower(Session::user().id).favorited(tid) ? "IS_FAVORITE" : "NOT_FAVORITE");

        if(Session::user()){
            DB::Result playlists = DB::query(
                "SELECT id, name FROM playlists WHERE user_id = " + number(Session::user().id) + " ORDER BY name ASC");
            if(!playlists.empty()){
                doc->dict()->ShowSection("HAS_PLAYLISTS");
                for(DB::Result::const_iterator i=playlists.begin(); i!=playlists.end(); i++){
                    Dict *playlist = doc->dict()->AddSectionDictionary("PLAYLIST");
                    playlist->SetValue("PLAYLIST_ID", i->at(0));
                    playlist->SetValue("PLAYLIST_NAME", i->at(1));
                }
            }
        }
    }

    else if(sub == "delete"){
        Track t(tid);
        if(!t) return;
        if(!t.artist.self())
            doc->redirect(t.url());

        else if(!post || cgi("confirm") != "Delete" || Session::nonce() != cgi("nonce")){
            Session::newNonce();
            doc->setHtml("html/delete.tpl", "Track deletion");
            doc->dict()->SetValue("WHAT", t.title);
            doc->dict()->SetValue("CANCEL_URL", t.url());
        }

        else{
            delete_track(t);
            doc->redirect(Session::user().url());
        }

    }

    else if(sub == "publish"){
        Track t(tid);
        if(!t) return;
        if(tid != number(cgi("tid")))
            return doc->redirect(t.url());

        if(t.artist.self() && !t.visible && post){
            publish_track(t);
        }
        doc->redirect(t.url());

    }
}

void Pages::JSONTrack(Document *doc){
    std::string sub;
    int tid = route("track", path, sub);
    bool post = cgi.getEnvironment().getRequestMethod() == "POST";

    if(!tid) return;

    if(sub == "json" || sub == "delete/json" || sub == "publish/json"){
        ExtendedTrack t(tid);
        if(!t){
            doc->setJson("json/error.tpl", 404);
            doc->dict()->SetValue("ERROR", "No such track.");
            return;
        }
        if(sub == "json"){
            doc->setJson("json/track.tpl");
            t.fill(doc->dict());
            doc->dict()->ShowSection("LONG");
            return;
        }
        if(!post){
            doc->setJson("json/error.tpl", 405);
            doc->dict()->SetValue("ERROR", "This resource can only be accessed with POST.");
            return;
        }
        if(!t.artist.self()){
            doc->setJson("json/error.tpl", 403);
            doc->dict()->SetValue("ERROR", "You do not own this track.");
            return;
        }
        if(sub == "delete/json"){
            delete_track(t);
            doc->setJson("json/ok.tpl");
        }
        else{
            if(!t.visible){
                publish_track(t);
                doc->setJson("json/ok.tpl");
            }
            else {
                doc->setJson("json/error.tpl", 200);
                doc->dict()->SetValue("ERROR", "Track has already been published.");
            }
        }

    }
}
