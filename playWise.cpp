/**
 * ============================================================================
 * PlayWise Music Player - Advanced Playlist Management System
 * ============================================================================
 * 
 * @file        playWise.cpp
 * @author      PlayWise Development Team
 * @version     2.0
 * @date        August 2025
 * @brief       A comprehensive music player with playlist management, smart
 *              auto-replay, skip tracking, and rating system
 * 
 * FEATURES:
 * - Doubly-linked list based playlist with O(1) insertion/deletion
 * - Stack-based playback history with undo functionality
 * - Tree-structured rating system for song organization
 * - Hash-based song lookup for O(1) average search time
 * - Circular buffer skip tracking with sliding window
 * - Smart auto-replay system with genre-based mood detection
 * - Recently added songs tracking with chronological order
 * - Comprehensive data persistence across sessions
 * 
 * OVERALL TIME COMPLEXITY: O(n log n) for sorting operations, O(n) for most
 * playlist operations, O(1) for song lookups and basic operations
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <deque>

using namespace std;

// Forward declarations
class RecentlySkippedTracker;

/**
 * ============================================================================
 * CORE DATA STRUCTURES
 * ============================================================================
 */

/**
 * @struct Song
 * @brief Represents a music track with metadata and doubly-linked connections
 * 
 * A node in the doubly-linked playlist structure containing song metadata
 * and pointers for bidirectional navigation.
 */
struct Song {
    string title;     ///< Song title
    string artist;    ///< Artist name
    string genre;     ///< Music genre for auto-replay classification
    int duration;     ///< Duration in seconds
    Song* prev;       ///< Previous song in playlist (doubly-linked)
    Song* next;       ///< Next song in playlist (doubly-linked)

    /**
     * @brief Primary constructor with genre support
     * @param t Song title
     * @param a Artist name
     * @param g Music genre
     * @param d Duration in seconds
     * @time_complexity O(1)
     */
    Song(string t, string a, string g, int d) 
        : title(t), artist(a), genre(g), duration(d), prev(nullptr), next(nullptr) {}
    
    /**
     * @brief Backward compatibility constructor (default genre to "Unknown")
     * @param t Song title
     * @param a Artist name
     * @param d Duration in seconds
     * @time_complexity O(1)
     */
    Song(string t, string a, int d) 
        : title(t), artist(a), genre("Unknown"), duration(d), prev(nullptr), next(nullptr) {}
};

/**
 * ============================================================================
 * PLAYLIST MANAGEMENT CLASS
 * ============================================================================
 */

/**
 * @class Playlist
 * @brief Doubly-linked list implementation for efficient playlist operations
 * 
 * Provides O(1) insertion/deletion at ends and O(n) for arbitrary positions.
 * Supports bidirectional navigation and playlist manipulation operations.
 */
class Playlist {
private:
    Song* head;  ///< First song in playlist
    Song* tail;  ///< Last song in playlist

public:
    /**
     * @brief Default constructor - initializes empty playlist
     * @time_complexity O(1)
     */
    Playlist() : head(nullptr), tail(nullptr) {}

    /**
     * @brief Destructor - safely deallocates all songs
     * @time_complexity O(n) where n = number of songs
     */
    ~Playlist() {
        while (head) {
            Song* temp = head;
            head = head->next;
            delete temp;
        }
    }

    /**
     * @brief Add song without genre (backward compatibility)
     * @param title Song title
     * @param artist Artist name
     * @param duration Duration in seconds
     * @return Pointer to newly created song
     * @time_complexity O(1) - insertion at tail
     */
    Song* add_song(const string& title, const string& artist, int duration) {
        Song* new_song = new Song(title, artist, duration);
        if (!head) {
            head = tail = new_song;
        } else {
            tail->next = new_song;
            new_song->prev = tail;
            tail = new_song;
        }
        return new_song;
    }

    /**
     * @brief Add song with genre support for auto-replay functionality
     * @param title Song title
     * @param artist Artist name
     * @param genre Music genre
     * @param duration Duration in seconds
     * @return Pointer to newly created song
     * @time_complexity O(1) - insertion at tail
     */
    Song* add_song(const string& title, const string& artist, const string& genre, int duration) {
        Song* new_song = new Song(title, artist, genre, duration);
        if (!head) {
            head = tail = new_song;
        } else {
            tail->next = new_song;
            new_song->prev = tail;
            tail = new_song;
        }
        return new_song;
    }

    /**
     * @brief Remove song at specified index
     * @param index Position to delete (0-based)
     * @time_complexity O(n) - linear search to index, O(1) deletion
     */
    void delete_song(int index) {
        if (!head) return;
        
        Song* temp = head;
        int i = 0;
        
        // Find the song at specified index
        while (temp && i < index) {
            temp = temp->next;
            i++;
        }
        
        if (!temp) return; // Index out of bounds
        
        // Update links
        if (temp->prev) temp->prev->next = temp->next;
        if (temp->next) temp->next->prev = temp->prev;
        if (temp == head) head = temp->next;
        if (temp == tail) tail = temp->prev;
        
        delete temp;
    }

    /**
     * @brief Move song from one position to another
     * @param from_index Source position
     * @param to_index Destination position
     * @time_complexity O(n) - linear search for positions, O(1) for link updates
     */
    void move_song(int from_index, int to_index) {
        if (from_index == to_index || !head) return;
        
        // Find song to move
        Song* song_to_move = head;
        int i = 0;
        while (song_to_move && i < from_index) {
            song_to_move = song_to_move->next;
            i++;
        }
        if (!song_to_move) return;
        
        // Remove from current position
        if (song_to_move->prev) song_to_move->prev->next = song_to_move->next;
        if (song_to_move->next) song_to_move->next->prev = song_to_move->prev;
        if (song_to_move == head) head = song_to_move->next;
        if (song_to_move == tail) tail = song_to_move->prev;
        
        song_to_move->prev = nullptr;
        song_to_move->next = nullptr;
        
        // Adjust target index if moving backwards
        if (to_index > from_index) to_index--;
        
        // Insert at new position
        if (to_index == 0) {
            // Insert at beginning
            song_to_move->next = head;
            if (head) head->prev = song_to_move;
            head = song_to_move;
            if (!tail) tail = song_to_move;
        } else {
            // Find insertion point
            Song* temp = head;
            i = 0;
            while (temp && i < to_index - 1) {
                temp = temp->next;
                i++;
            }
            
            if (!temp) {
                // Insert at end
                if (tail) {
                    tail->next = song_to_move;
                    song_to_move->prev = tail;
                    tail = song_to_move;
                } else {
                    head = tail = song_to_move;
                }
            } else {
                // Insert after temp
                song_to_move->next = temp->next;
                song_to_move->prev = temp;
                if (temp->next) temp->next->prev = song_to_move;
                else tail = song_to_move;
                temp->next = song_to_move;
            }
        }
    }

    /**
     * @brief Reverse entire playlist order
     * @time_complexity O(n) - single pass through all nodes
     */
    void reverse_playlist() {
        Song* curr = head;
        while (curr) {
            swap(curr->next, curr->prev);
            curr = curr->prev; // Note: prev is now next due to swap
        }
        swap(head, tail);
    }

    /**
     * @brief Get all songs as vector for iteration
     * @return Vector containing pointers to all songs
     * @time_complexity O(n) - single traversal
     */
    vector<Song*> get_all_songs() {
        vector<Song*> songs;
        Song* temp = head;
        while (temp) {
            songs.push_back(temp);
            temp = temp->next;
        }
        return songs;
    }
};

/**
 * ============================================================================
 * PLAYBACK HISTORY MANAGEMENT
 * ============================================================================
 */

/**
 * @class PlaybackHistory
 * @brief Stack-based history for undo functionality and recent song tracking
 * 
 * Maintains playback history using LIFO stack for efficient undo operations.
 */
class PlaybackHistory {
private:
    stack<Song*> history;  ///< Stack of recently played songs

public:
    /**
     * @brief Add song to playback history
     * @param song Pointer to played song
     * @time_complexity O(1) - stack push operation
     */
    void add(Song* song) {
        history.push(song);
    }

    /**
     * @brief Undo last play operation (remove from history)
     * @return Pointer to last played song, nullptr if empty
     * @time_complexity O(1) - stack pop operation
     */
    Song* undo_last_play() {
        if (history.empty()) return nullptr;
        Song* song = history.top();
        history.pop();
        return song;
    }

    /**
     * @brief Get recently played songs for display
     * @param n Number of recent songs to retrieve (default: 5)
     * @return Vector of recently played songs
     * @time_complexity O(min(n, stack_size)) - temporary stack operations
     */
    vector<Song*> get_recently_played(int n = 5) {
        vector<Song*> recent;
        stack<Song*> temp = history;
        
        while (!temp.empty() && static_cast<int>(recent.size()) < n) {
            recent.push_back(temp.top());
            temp.pop();
        }
        return recent;
    }
};

/**
 * ============================================================================
 * RATING SYSTEM MANAGEMENT
 * ============================================================================
 */

/**
 * @class SongRatingTree
 * @brief Map-based rating system for song organization by rating
 * 
 * Uses balanced BST (std::map) for efficient rating-based song retrieval.
 */
class SongRatingTree {
private:
    map<int, vector<Song*>> ratingMap;  ///< Rating -> Songs mapping

public:
    /**
     * @brief Insert song with rating
     * @param song Pointer to song
     * @param rating Rating value (1-5)
     * @time_complexity O(log k + 1) where k = number of distinct ratings
     */
    void insert_song(Song* song, int rating) {
        ratingMap[rating].push_back(song);
    }

    /**
     * @brief Search songs by rating
     * @param rating Target rating
     * @return Vector of songs with specified rating
     * @time_complexity O(log k) where k = number of distinct ratings
     */
    vector<Song*> search_by_rating(int rating) {
        return ratingMap[rating];
    }

    /**
     * @brief Remove song with specific rating
     * @param song Pointer to song to remove
     * @param rating Rating of the song
     * @time_complexity O(log k + m) where k = ratings, m = songs with that rating
     */
    void delete_song(Song* song, int rating) {
        auto& vec = ratingMap[rating];
        vec.erase(remove(vec.begin(), vec.end(), song), vec.end());
    }

    /**
     * @brief Get count of songs per rating for statistics
     * @return Map of rating -> count
     * @time_complexity O(k) where k = number of distinct ratings
     */
    map<int, int> get_song_count_by_rating() {
        map<int, int> count;
        for (auto& pair : ratingMap) {
            count[pair.first] = pair.second.size();
        }
        return count;
    }
};

/**
 * ============================================================================
 * SONG LOOKUP SYSTEM
 * ============================================================================
 */

/**
 * @class SongLookup
 * @brief Hash-based O(1) song lookup by title
 * 
 * Provides fast song retrieval using unordered_map for title-based searches.
 */
class SongLookup {
private:
    unordered_map<string, Song*> lookup;  ///< Title -> Song pointer mapping

public:
    /**
     * @brief Add song to lookup table
     * @param song Pointer to song
     * @time_complexity O(1) average case for hash insertion
     */
    void add(Song* song) {
        lookup[song->title] = song;
    }

    /**
     * @brief Get song by title
     * @param title Song title to search
     * @return Pointer to song if found, nullptr otherwise
     * @time_complexity O(1) average case for hash lookup
     */
    Song* get(const string& title) {
        return lookup.count(title) ? lookup[title] : nullptr;
    }
};

/**
 * ============================================================================
 * SKIP TRACKING SYSTEM
 * ============================================================================
 */

/**
 * @class RecentlySkippedTracker
 * @brief Circular buffer implementation for tracking recently skipped songs
 * 
 * Maintains a sliding window of last 10 skipped songs using deque for
 * efficient insertion/deletion at both ends. Prevents recently skipped
 * songs from being replayed in auto-replay mode.
 */
class RecentlySkippedTracker {
private:
    deque<Song*> skippedSongs;      ///< Circular buffer of skipped songs
    static const int MAX_SKIPPED = 10;  ///< Maximum songs to track

public:
    /**
     * @brief Add song to skip history (circular buffer with sliding window)
     * @param song Pointer to skipped song
     * @time_complexity O(n) where n ≤ 10 for duplicate check, O(1) for operations
     */
    void addSkippedSong(Song* song) {
        // Remove existing entry to avoid duplicates and maintain recency
        auto it = find(skippedSongs.begin(), skippedSongs.end(), song);
        if (it != skippedSongs.end()) {
            skippedSongs.erase(it);
        }
        
        // Add to front (most recent skip)
        skippedSongs.push_front(song);
        
        // Maintain sliding window size
        if (skippedSongs.size() > MAX_SKIPPED) {
            skippedSongs.pop_back(); // Remove oldest skipped song
        }
    }

    /**
     * @brief Check if song was recently skipped
     * @param song Pointer to song to check
     * @return True if recently skipped, false otherwise
     * @time_complexity O(k) where k ≤ 10 (linear search in small buffer)
     */
    bool isRecentlySkipped(Song* song) {
        return find(skippedSongs.begin(), skippedSongs.end(), song) != skippedSongs.end();
    }

    /**
     * @brief Get all recently skipped songs for display
     * @return Vector of skipped songs (most recent first)
     * @time_complexity O(k) where k ≤ 10 (copy from deque to vector)
     */
    vector<Song*> getSkippedSongs() {
        return vector<Song*>(skippedSongs.begin(), skippedSongs.end());
    }

    /**
     * @brief Clear all skip history
     * @time_complexity O(1) - deque clear operation
     */
    void clearSkippedHistory() {
        skippedSongs.clear();
        cout << "🗑️  Cleared all skipped songs history." << endl;
    }

    /**
     * @brief Get current count of tracked skipped songs
     * @return Number of songs in skip history
     * @time_complexity O(1) - deque size operation
     */
    int getSkippedCount() {
        return skippedSongs.size();
    }
};

/**
 * ============================================================================
 * RECENTLY ADDED TRACKING SYSTEM
 * ============================================================================
 */

/**
 * @class RecentlyAddedTracker
 * @brief Tracks recently added songs with chronological order
 * 
 * Maintains a chronological list of recently added songs using deque for
 * efficient insertion and provides quick access to newest additions.
 */
class RecentlyAddedTracker {
private:
    deque<Song*> recentlyAdded;     ///< Chronological list of recently added songs
    static const int MAX_RECENT = 15;  ///< Maximum songs to track (increased for better history)

public:
    /**
     * @brief Add song to recently added history
     * @param song Pointer to newly added song
     * @time_complexity O(n) where n ≤ 15 for duplicate check, O(1) for operations
     */
    void addRecentSong(Song* song) {
        // Remove existing entry to avoid duplicates and maintain recency
        auto it = find(recentlyAdded.begin(), recentlyAdded.end(), song);
        if (it != recentlyAdded.end()) {
            recentlyAdded.erase(it);
        }
        
        // Add to front (most recently added)
        recentlyAdded.push_front(song);
        
        // Maintain sliding window size
        if (recentlyAdded.size() > MAX_RECENT) {
            recentlyAdded.pop_back(); // Remove oldest added song
        }
    }

    /**
     * @brief Check if song was recently added
     * @param song Pointer to song to check
     * @return True if recently added, false otherwise
     * @time_complexity O(k) where k ≤ 15 (linear search in small buffer)
     */
    bool isRecentlyAdded(Song* song) {
        return find(recentlyAdded.begin(), recentlyAdded.end(), song) != recentlyAdded.end();
    }

    /**
     * @brief Get all recently added songs for display
     * @param limit Maximum number of songs to return (default: 10)
     * @return Vector of recently added songs (most recent first)
     * @time_complexity O(min(limit, k)) where k ≤ 15
     */
    vector<Song*> getRecentlyAdded(int limit = 10) {
        vector<Song*> result;
        int count = min(limit, static_cast<int>(recentlyAdded.size()));
        
        for (int i = 0; i < count; ++i) {
            result.push_back(recentlyAdded[i]);
        }
        
        return result;
    }

    /**
     * @brief Get the most recently added song
     * @return Pointer to most recently added song, nullptr if empty
     * @time_complexity O(1)
     */
    Song* getLastAdded() {
        return recentlyAdded.empty() ? nullptr : recentlyAdded.front();
    }

    /**
     * @brief Clear all recently added history
     * @time_complexity O(1) - deque clear operation
     */
    void clearRecentlyAdded() {
        recentlyAdded.clear();
        cout << "🗑️  Cleared recently added songs history." << endl;
    }

    /**
     * @brief Get current count of tracked recently added songs
     * @return Number of songs in recently added history
     * @time_complexity O(1) - deque size operation
     */
    int getRecentCount() {
        return recentlyAdded.size();
    }

    /**
     * @brief Get top N most recently added songs of a specific genre
     * @param genre Target genre to filter by
     * @param limit Maximum number of songs to return (default: 5)
     * @return Vector of recently added songs matching the genre
     * @time_complexity O(k) where k ≤ 15 (linear search through recent songs)
     */
    vector<Song*> getRecentlyAddedByGenre(const string& genre, int limit = 5) {
        vector<Song*> result;
        
        for (auto* song : recentlyAdded) {
            if (song->genre == genre) {
                result.push_back(song);
                if (static_cast<int>(result.size()) >= limit) break;
            }
        }
        
        return result;
    }
};

/**
 * ============================================================================
 * PLAYLIST PLAYER SYSTEM
 * ============================================================================
 */

/**
 * @class PlaylistPlayer
 * @brief Sequential playlist playback with navigation controls
 * 
 * Manages current playback position and provides next/previous navigation
 * with automatic end-of-playlist detection for auto-replay triggering.
 */
class PlaylistPlayer {
private:
    int currentIndex;       ///< Current song position in playlist
    bool isPlaying;         ///< Playback state flag
    Song* currentSong;      ///< Pointer to currently playing song

public:
    /**
     * @brief Initialize player in stopped state
     * @time_complexity O(1)
     */
    PlaylistPlayer() : currentIndex(-1), isPlaying(false), currentSong(nullptr) {}

    /**
     * @brief Play entire playlist sequentially from start to finish
     * @param playlist Reference to playlist
     * @param ph Reference to playback history
     * @param playCounts Reference to play count tracking
     * @time_complexity O(n) where n = number of songs in playlist
     */
    void playEntirePlaylist(Playlist& playlist, PlaybackHistory& ph, unordered_map<string, int>& playCounts) {
        auto songs = playlist.get_all_songs();
        if (songs.empty()) {
            cout << "❌ Playlist is empty!" << endl;
            return;
        }
        
        cout << "\n🎵 Playing entire playlist (" << songs.size() << " songs)...\n";
        cout << "==========================================\n";
        
        for (size_t i = 0; i < songs.size(); ++i) {
            currentIndex = i;
            currentSong = songs[i];
            isPlaying = true;
            
            ph.add(currentSong);
            playCounts[currentSong->title]++;
            
            cout << "▶️  [" << (i+1) << "/" << songs.size() << "] " 
                 << currentSong->title << " by " << currentSong->artist 
                 << " (" << currentSong->duration << "s)" << endl;
        }
        
        cout << "==========================================\n";
        cout << "✅ Playlist finished! Checking for auto-replay...\n";
        isPlaying = false;
    }

    /**
     * @brief Play next song in playlist sequence
     * @param playlist Reference to playlist
     * @param ph Reference to playback history
     * @param playCounts Reference to play count tracking
     * @return True if successful, false if end of playlist reached
     * @time_complexity O(n) for getting all songs, O(1) for navigation
     */
    bool playNext(Playlist& playlist, PlaybackHistory& ph, unordered_map<string, int>& playCounts) {
        auto songs = playlist.get_all_songs();
        if (songs.empty()) {
            cout << "❌ Playlist is empty!" << endl;
            return false;
        }
        
        if (currentIndex + 1 >= static_cast<int>(songs.size())) {
            cout << "🔚 Reached end of playlist!" << endl;
            return false;
        }
        
        currentIndex++;
        currentSong = songs[currentIndex];
        isPlaying = true;
        
        ph.add(currentSong);
        playCounts[currentSong->title]++;
        
        cout << "⏭️  Next: [" << (currentIndex+1) << "/" << songs.size() << "] " 
             << currentSong->title << " by " << currentSong->artist 
             << " (" << currentSong->duration << "s)" << endl;
             
        return true;
    }

    /**
     * @brief Play previous song in playlist sequence
     * @param playlist Reference to playlist
     * @param ph Reference to playback history
     * @param playCounts Reference to play count tracking
     * @return True if successful, false if at beginning of playlist
     * @time_complexity O(n) for getting all songs, O(1) for navigation
     */
    bool playPrevious(Playlist& playlist, PlaybackHistory& ph, unordered_map<string, int>& playCounts) {
        auto songs = playlist.get_all_songs();
        if (songs.empty()) {
            cout << "❌ Playlist is empty!" << endl;
            return false;
        }
        
        if (currentIndex <= 0) {
            cout << "🔙 Already at the beginning of playlist!" << endl;
            return false;
        }
        
        currentIndex--;
        currentSong = songs[currentIndex];
        isPlaying = true;
        
        ph.add(currentSong);
        playCounts[currentSong->title]++;
        
        cout << "⏮️  Previous: [" << (currentIndex+1) << "/" << songs.size() << "] " 
             << currentSong->title << " by " << currentSong->artist 
             << " (" << currentSong->duration << "s)" << endl;
             
        return true;
    }

    /**
     * @brief Display current song information
     * @param playlist Reference to playlist for position info
     * @time_complexity O(n) for getting all songs, O(1) for display
     */
    void showCurrentSong(Playlist& playlist) {
        auto songs = playlist.get_all_songs();
        if (currentSong && isPlaying && currentIndex >= 0 && currentIndex < static_cast<int>(songs.size())) {
            cout << "\n🎵 Currently Playing:\n";
            cout << "📀 Song: " << currentSong->title << endl;
            cout << "🎤 Artist: " << currentSong->artist << endl;
            cout << "🎧 Genre: " << currentSong->genre << endl;
            cout << "⏱️  Duration: " << currentSong->duration << "s" << endl;
            cout << "📊 Position: " << (currentIndex+1) << "/" << songs.size() << endl;
        } else {
            cout << "⏸️  No song currently playing." << endl;
        }
    }

    // Getters for state access (O(1) operations)
    bool getIsPlaying() { return isPlaying; }
    int getCurrentIndex() { return currentIndex; }
    Song* getCurrentSong() { return currentSong; }
};

/**
 * ============================================================================
 * AUTO-REPLAY SYSTEM
 * ============================================================================
 */

/**
 * @class AutoReplaySystem
 * @brief Intelligent auto-replay with genre-based mood detection
 * 
 * Automatically selects calming songs when playlist ends, filtering out
 * recently skipped tracks to provide a pleasant listening experience.
 */
class AutoReplaySystem {
private:
    /// Predefined calming genres for mood-based selection
    vector<string> calmingGenres = {"Lo-Fi", "Jazz", "Classical", "Ambient", "Chill", "Lofi"};

public:
    /**
     * @brief Check if genre is classified as calming
     * @param genre Genre string to check
     * @return True if calming, false otherwise
     * @time_complexity O(g) where g = number of calming genres (small constant)
     */
    bool isCalming(const string& genre) {
        string lowerGenre = genre;
        transform(lowerGenre.begin(), lowerGenre.end(), lowerGenre.begin(), ::tolower);
        
        for (const auto& calming : calmingGenres) {
            string lowerCalming = calming;
            transform(lowerCalming.begin(), lowerCalming.end(), lowerCalming.begin(), ::tolower);
            if (lowerGenre == lowerCalming) return true;
        }
        return false;
    }

    /**
     * @brief Get top 3 most-played calming songs for auto-replay
     * @param allSongs Vector of all available songs
     * @param playCounts Map of song title to play count
     * @param skipTracker Reference to skip tracker for filtering
     * @return Vector of top 3 calming songs (excluding recently skipped)
     * @time_complexity O(n log n) where n = number of calming songs (for sorting)
     */
    vector<Song*> getTop3CalmingSongs(const vector<Song*>& allSongs, 
                                     const unordered_map<string, int>& playCounts,
                                     RecentlySkippedTracker& skipTracker) {
        vector<pair<int, Song*>> calmingSongs;
        
        // Filter calming songs and exclude recently skipped
        for (auto* song : allSongs) {
            if (isCalming(song->genre) && !skipTracker.isRecentlySkipped(song)) {
                int playCount = playCounts.count(song->title) ? playCounts.at(song->title) : 0;
                calmingSongs.push_back({playCount, song});
            }
        }
        
        if (calmingSongs.empty()) {
            cout << "🔇 No calming songs found for auto-replay (or all are recently skipped)." << endl;
            return {};
        }
        
        // Sort by play count (most played first)
        sort(calmingSongs.begin(), calmingSongs.end(), greater<pair<int, Song*>>());
        
        // Return top 3
        vector<Song*> result;
        for (int i = 0; i < min(3, (int)calmingSongs.size()); ++i) {
            result.push_back(calmingSongs[i].second);
        }
        
        return result;
    }

    /**
     * @brief Start auto-replay with selected calming songs
     * @param calmingSongs Vector of songs to play in auto-replay
     * @param ph Reference to playback history
     * @param playCounts Reference to play count tracking
     * @time_complexity O(k) where k = number of calming songs (typically 3)
     */
    void startAutoReplay(vector<Song*> calmingSongs, PlaybackHistory& ph, 
                        unordered_map<string, int>& playCounts) {
        if (calmingSongs.empty()) return;
        
        cout << "\n🔄 Auto-Replay: Starting calming songs loop..." << endl;
        cout << "🎵 Playing top " << calmingSongs.size() << " most-played calming songs:" << endl;
        
        for (auto* song : calmingSongs) {
            ph.add(song);
            playCounts[song->title]++;
            cout << "🎶 " << song->title << " (" << song->genre << ") - " 
                 << playCounts[song->title] << " plays" << endl;
        }
        
        cout << "💭 Auto-replay complete. Songs will continue looping until you play something else." << endl;
    }
};

/**
 * ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Compare songs by title (ascending)
 * @time_complexity O(m) where m = average string length
 */
bool compare_title(Song* a, Song* b) {
    return a->title < b->title;
}

/**
 * @brief Compare songs by duration (ascending)
 * @time_complexity O(1)
 */
bool compare_duration(Song* a, Song* b) {
    return a->duration < b->duration;
}

/**
 * @brief Sort songs by specified criteria
 * @param songs Vector of song pointers to sort
 * @param by Sort criteria ("title" or "duration")
 * @time_complexity O(n log n) where n = number of songs
 */
void sort_songs(vector<Song*>& songs, string by) {
    if (by == "title") {
        sort(songs.begin(), songs.end(), compare_title);
    } else if (by == "duration") {
        sort(songs.begin(), songs.end(), compare_duration);
    }
}

/**
 * @brief Generate comprehensive system analytics snapshot
 * @param all_songs Vector of all songs
 * @param ph Reference to playback history
 * @param srt Reference to rating tree
 * @param playCounts Reference to play counts
 * @time_complexity O(n log n) for sorting + O(n) for other operations
 */
void export_snapshot(vector<Song*> all_songs, PlaybackHistory& ph, SongRatingTree& srt, 
                     unordered_map<string, int>& playCounts) {
    cout << "\n=== SYSTEM SNAPSHOT ===\n";
    
    // Sort by duration for top longest songs
    sort(all_songs.begin(), all_songs.end(), [](Song* a, Song* b) {
        return a->duration > b->duration;
    });
    
    cout << "Top 5 Longest Songs:\n";
    for (int i = 0; i < min(5, (int)all_songs.size()); ++i) {
        cout << all_songs[i]->title << " - " << all_songs[i]->duration << "s\n";
    }
    
    cout << "\nRecently Played:\n";
    for (auto* s : ph.get_recently_played()) {
        cout << s->title << "\n";
    }
    
    cout << "\nSong Count by Rating:\n";
    for (auto& pair : srt.get_song_count_by_rating()) {
        cout << pair.first << " stars: " << pair.second << " songs\n";
    }
    
    cout << "\nPlay Count for Songs:\n";
    for (auto& pair : playCounts) {
        cout << pair.first << " → " << pair.second << " plays\n";
    }
    cout << "========================\n";
}

/**
 * ============================================================================
 * DATA PERSISTENCE SYSTEM
 * ============================================================================
 */

/**
 * @brief Save all system data to file in structured format
 * @param songs Vector of all songs
 * @param playCounts Map of play counts
 * @param srt Reference to rating tree
 * @param ph Reference to playback history
 * @param skipTracker Reference to skip tracker
 * @param recentTracker Reference to recently added tracker
 * @time_complexity O(n + r + h + s + a) where n=songs, r=ratings, h=history, s=skipped, a=recent
 */
void save_all_data(const vector<Song*>& songs, const unordered_map<string, int>& playCounts, 
                   SongRatingTree& srt, PlaybackHistory& ph, RecentlySkippedTracker& skipTracker,
                   RecentlyAddedTracker& recentTracker) {
    ofstream file("playwise_data.txt");
    
    // Save songs section with full metadata
    file << "[SONGS]\n";
    for (auto* s : songs) {
        file << s->title << "," << s->artist << "," << s->genre << "," << s->duration << "\n";
    }
    
    // Save play counts section
    file << "[PLAY_COUNTS]\n";
    for (auto& pair : playCounts) {
        file << pair.first << "," << pair.second << "\n";
    }
    
    // Save ratings section
    file << "[RATINGS]\n";
    auto ratingCounts = srt.get_song_count_by_rating();
    for (auto& pair : ratingCounts) {
        if (pair.second > 0) {
            auto songsWithRating = srt.search_by_rating(pair.first);
            for (auto* song : songsWithRating) {
                file << song->title << "," << pair.first << "\n";
            }
        }
    }
    
    // Save recently played history section
    file << "[HISTORY]\n";
    auto recentSongs = ph.get_recently_played();
    for (auto* song : recentSongs) {
        file << song->title << "\n";
    }
    
    // Save recently skipped songs section
    file << "[SKIPPED]\n";
    auto skippedSongs = skipTracker.getSkippedSongs();
    for (auto* song : skippedSongs) {
        file << song->title << "\n";
    }
    
    // Save recently added songs section
    file << "[RECENT_ADDED]\n";
    auto recentlyAddedSongs = recentTracker.getRecentlyAdded(15); // Save all tracked songs
    for (auto* song : recentlyAddedSongs) {
        file << song->title << "\n";
    }
    
    file << "[END]\n";
    file.close();
}

/**
 * @brief Load all system data from file with comprehensive error handling
 * @param playlist Reference to playlist
 * @param lookup Reference to song lookup
 * @param playCounts Reference to play counts
 * @param srt Reference to rating tree
 * @param ph Reference to playback history
 * @param skipTracker Reference to skip tracker
 * @param recentTracker Reference to recently added tracker
 * @time_complexity O(n + r + h + s + a) where n=songs, r=ratings, h=history, s=skipped, a=recent
 */
void load_all_data(Playlist& playlist, SongLookup& lookup, unordered_map<string, int>& playCounts, 
                   SongRatingTree& srt, PlaybackHistory& ph, RecentlySkippedTracker& skipTracker,
                   RecentlyAddedTracker& recentTracker) {
    ifstream file("playwise_data.txt");
    if (!file.is_open()) {
        cout << "📁 No previous data file found. Starting fresh." << endl;
        return;
    }
    
    string line, section;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        // Detect section headers
        if (line[0] == '[' && line.back() == ']') {
            section = line;
            continue;
        }
        
        // Process each section
        if (section == "[SONGS]") {
            stringstream ss(line);
            string title, artist, genre, duration_str;
            getline(ss, title, ',');
            getline(ss, artist, ',');
            getline(ss, genre, ',');
            getline(ss, duration_str, ',');
            
            int duration = stoi(duration_str);
            Song* newSong = playlist.add_song(title, artist, genre, duration);
            
            // Add to lookup immediately for cross-referencing
            lookup.add(newSong);
        }
        else if (section == "[PLAY_COUNTS]") {
            stringstream ss(line);
            string title, count_str;
            getline(ss, title, ',');
            getline(ss, count_str, ',');
            playCounts[title] = stoi(count_str);
        }
        else if (section == "[RATINGS]") {
            stringstream ss(line);
            string title, rating_str;
            getline(ss, title, ',');
            getline(ss, rating_str, ',');
            Song* song = lookup.get(title);
            if (song) {
                srt.insert_song(song, stoi(rating_str));
            }
        }
        else if (section == "[HISTORY]") {
            Song* song = lookup.get(line);
            if (song) {
                ph.add(song);
            }
        }
        else if (section == "[SKIPPED]") {
            Song* song = lookup.get(line);
            if (song) {
                skipTracker.addSkippedSong(song);
            }
        }
        else if (section == "[RECENT_ADDED]") {
            Song* song = lookup.get(line);
            if (song) {
                recentTracker.addRecentSong(song);
            }
        }
    }
    file.close();
    cout << "✅ Successfully loaded data from previous session." << endl;
}

/**
 * ============================================================================
 * MAIN APPLICATION CONTROLLER
 * ============================================================================
 */

/**
 * @brief Main application entry point with comprehensive menu system
 * @return Exit status (0 for success)
 * @time_complexity Varies by operation: O(1) to O(n log n) depending on user choice
 * 
 * MENU OPERATIONS TIME COMPLEXITY:
 * 1. Add Song: O(1)
 * 2. Delete Song: O(n)
 * 3. Move Song: O(n)
 * 4. Reverse Playlist: O(n)
 * 5. Undo Last Play: O(1)
 * 6. Search Song: O(1) average
 * 7. Insert Rating: O(log k)
 * 8. View by Rating: O(log k)
 * 9. Export Snapshot: O(n log n)
 * 10. Sort Songs: O(n log n)
 * 11. Play Song: O(1) average
 * 12. Play Playlist: O(n)
 * 13. Next Song: O(n)
 * 14. Previous Song: O(n)
 * 15. Show Current: O(n)
 * 16. Skip Song: O(1) average + O(10) for skip tracking
 * 17. View Skip History: O(10)
 * 18. Clear Skip History: O(1)
 * 19. View Recently Added: O(15) for display
 * 20. Clear Recently Added: O(1)
 */
int main() {
    // Initialize all system components
    Playlist playlist;
    PlaybackHistory ph;
    SongRatingTree srt;
    SongLookup lookup;
    unordered_map<string, int> playCounts;
    
    // Advanced feature components
    PlaylistPlayer player;
    AutoReplaySystem autoReplay;
    RecentlySkippedTracker skipTracker;
    RecentlyAddedTracker recentTracker;  // New recently added tracker

    // Load persisted data from previous session
    load_all_data(playlist, lookup, playCounts, srt, ph, skipTracker, recentTracker);
    
    // Helper function for auto-saving data after critical operations
    auto autoSave = [&]() {
        save_all_data(playlist.get_all_songs(), playCounts, srt, ph, skipTracker, recentTracker);
    };

    int choice;
    do {
        // Display comprehensive menu
        cout << "\n\n=== PlayWise Music Player v2.0 ===\n";
        cout << "📀 PLAYLIST MANAGEMENT:\n";
        cout << "1. Add Song                2. Delete Song\n";
        cout << "3. Move Song               4. Reverse Playlist\n";
        cout << "5. Undo Last Play\n\n";
        
        cout << "🔍 SEARCH & RATING:\n";
        cout << "6. Search Song by Title    7. Insert Song Rating\n";
        cout << "8. View Songs by Rating    9. Export System Snapshot\n";
        cout << "10. Sort Songs\n\n";
        
        cout << "▶️ PLAYBACK CONTROLS:\n";
        cout << "11. Play Individual Song   12. Play Entire Playlist\n";
        cout << "13. Play Next Song         14. Play Previous Song\n";
        cout << "15. Show Current Song\n\n";
        
        cout << "⏭️ SKIP MANAGEMENT:\n";
        cout << "16. Skip Song              17. View Skip History\n";
        cout << "18. Clear Skip History\n\n";
        
        cout << "🆕 RECENTLY ADDED:\n";
        cout << "19. View Recently Added    20. Clear Recently Added\n\n";
        
        cout << "0. Exit\nChoice: ";
        
        if (!(cin >> choice)) {
            cout << "❌ Invalid input. Exiting...\n";
            break;
        }

        switch (choice) {
            case 1: {
                // Add Song with Genre Support
                string title, artist, genre;
                int duration;
                cin.ignore();
                cout << "📝 Enter title: "; getline(cin, title);
                cout << "🎤 Enter artist: "; getline(cin, artist);
                cout << "🎧 Enter genre: "; getline(cin, genre);
                cout << "⏱️ Enter duration (seconds): "; cin >> duration;
                
                // Add song and get pointer to newly created song
                Song* newSong = playlist.add_song(title, artist, genre, duration);
                
                // Add the new song to lookup table immediately
                lookup.add(newSong);
                
                // Add to recently added tracker
                recentTracker.addRecentSong(newSong);
                
                // Auto-save data immediately to prevent data loss
                autoSave();
                
                cout << "✅ Song '" << title << "' added successfully and saved!" << endl;
                cout << "🆕 Added to recently added list (Total: " << recentTracker.getRecentCount() << "/15)" << endl;
                break;
            }
            
            case 2: {
                // Delete Song by Index
                int index;
                cout << "🗑️ Enter index to delete: "; cin >> index;
                playlist.delete_song(index);
                
                // Auto-save data after deletion
                autoSave();
                
                cout << "✅ Song deleted (if index was valid) and saved!" << endl;
                break;
            }
            
            case 3: {
                // Move Song Position
                int from, to;
                cout << "📤 Move from index: "; cin >> from;
                cout << "📥 To index: "; cin >> to;
                playlist.move_song(from, to);
                cout << "✅ Song moved successfully!" << endl;
                break;
            }
            
            case 4: {
                // Reverse Entire Playlist
                playlist.reverse_playlist();
                cout << "🔄 Playlist reversed successfully!" << endl;
                break;
            }
            
            case 5: {
                // Undo Last Play Operation
                Song* undone = ph.undo_last_play();
                if (undone) {
                    cout << "↩️ Undone last play: " << undone->title << endl;
                } else {
                    cout << "❌ No playback history available." << endl;
                }
                break;
            }
            
            case 6: {
                // Search Song by Title
                string title;
                cin.ignore();
                cout << "🔍 Enter title to search: "; getline(cin, title);
                Song* song = lookup.get(title);
                if (song) {
                    cout << "✅ Found: " << song->title << " by " << song->artist 
                         << " (" << song->genre << ")" << endl;
                } else {
                    cout << "❌ Song not found." << endl;
                }
                break;
            }
            
            case 7: {
                // Insert Song Rating
                string title;
                int rating;
                cin.ignore();
                cout << "🎵 Enter song title: "; getline(cin, title);
                cout << "⭐ Enter rating (1-5): "; cin >> rating;
                
                Song* song = lookup.get(title);
                if (song && rating >= 1 && rating <= 5) {
                    srt.insert_song(song, rating);
                    
                    // Auto-save data after rating
                    autoSave();
                    
                    cout << "✅ Rating saved successfully!" << endl;
                } else {
                    cout << "❌ Song not found or invalid rating." << endl;
                }
                break;
            }
            
            case 8: {
                // View Songs by Rating
                int rating;
                cout << "⭐ Enter rating to view (1-5): "; cin >> rating;
                auto songs = srt.search_by_rating(rating);
                
                if (songs.empty()) {
                    cout << "❌ No songs found with " << rating << " stars." << endl;
                } else {
                    cout << "\n🎵 Songs with " << rating << " stars:\n";
                    for (auto* s : songs) {
                        cout << "• " << s->title << " by " << s->artist << endl;
                    }
                }
                break;
            }
            
            case 9: {
                // Export System Snapshot
                export_snapshot(playlist.get_all_songs(), ph, srt, playCounts);
                break;
            }
            
            case 10: {
                // Sort Songs
                string criteria;
                cout << "📊 Sort by (title/duration): "; cin >> criteria;
                auto songs = playlist.get_all_songs();
                sort_songs(songs, criteria);
                
                cout << "\n📋 Sorted Songs:\n";
                for (auto* s : songs) {
                    cout << "• " << s->title << " - " << s->duration << "s (" 
                         << s->genre << ")" << endl;
                }
                break;
            }
            
            case 11: {
                // Play Individual Song
                string title;
                cin.ignore();
                cout << "🎵 Enter title to play: "; getline(cin, title);
                Song* song = lookup.get(title);
                
                if (song) {
                    ph.add(song);
                    playCounts[title]++;
                    
                    // Auto-save data after play count update
                    autoSave();
                    
                    cout << "\n▶️ Now Playing: " << song->title << " by " << song->artist 
                         << " (" << song->genre << ")" << endl;
                    cout << "🔢 Play count: " << playCounts[title] << endl;
                } else {
                    cout << "❌ Song not found." << endl;
                }
                break;
            }
            
            case 12: {
                // Play Entire Playlist with Auto-Replay
                player.playEntirePlaylist(playlist, ph, playCounts);
                
                // Auto-save data after playlist completion
                autoSave();
                
                // Trigger auto-replay with calming songs
                auto calmingSongs = autoReplay.getTop3CalmingSongs(
                    playlist.get_all_songs(), playCounts, skipTracker);
                if (!calmingSongs.empty()) {
                    autoReplay.startAutoReplay(calmingSongs, ph, playCounts);
                    // Save again after auto-replay
                    autoSave();
                }
                break;
            }
            
            case 13: {
                // Play Next Song
                if (!player.playNext(playlist, ph, playCounts)) {
                    // End of playlist - trigger auto-replay
                    auto calmingSongs = autoReplay.getTop3CalmingSongs(
                        playlist.get_all_songs(), playCounts, skipTracker);
                    if (!calmingSongs.empty()) {
                        cout << "\n🔄 End of playlist detected!" << endl;
                        autoReplay.startAutoReplay(calmingSongs, ph, playCounts);
                        autoSave();
                    }
                } else {
                    // Auto-save after successful next song
                    autoSave();
                }
                break;
            }
            
            case 14: {
                // Play Previous Song
                if (player.playPrevious(playlist, ph, playCounts)) {
                    // Auto-save after successful previous song
                    autoSave();
                }
                break;
            }
            
            case 15: {
                // Show Current Song Information
                player.showCurrentSong(playlist);
                break;
            }
            
            case 16: {
                // Skip Song and Add to Skip Tracker
                string title;
                cin.ignore();
                cout << "⏭️ Enter title to skip: "; getline(cin, title);
                Song* song = lookup.get(title);
                
                if (song) {
                    skipTracker.addSkippedSong(song);
                    
                    // Auto-save data after skip
                    autoSave();
                    
                    cout << "⏭️ Skipped: " << song->title << " (" << song->genre << ")" << endl;
                    cout << "📝 Total skipped songs: " << skipTracker.getSkippedCount() << "/10" << endl;
                } else {
                    cout << "❌ Song not found." << endl;
                }
                break;
            }
            
            case 17: {
                // View Skip History
                cout << "\n📜 Recently Skipped Songs (Last " << skipTracker.getSkippedCount() << "/10):\n";
                cout << "==========================================\n";
                auto skipped = skipTracker.getSkippedSongs();
                
                if (skipped.empty()) {
                    cout << "🔇 No songs have been skipped recently." << endl;
                } else {
                    for (size_t i = 0; i < skipped.size(); ++i) {
                        cout << (i+1) << ". " << skipped[i]->title << " by " << skipped[i]->artist 
                             << " (" << skipped[i]->genre << ")" << endl;
                    }
                }
                cout << "==========================================\n";
                break;
            }
            
            case 18: {
                // Clear Skip History
                skipTracker.clearSkippedHistory();
                break;
            }
            
            case 19: {
                // View Recently Added Songs
                cout << "\n🆕 Recently Added Songs (Last " << recentTracker.getRecentCount() << "/15):\n";
                cout << "==========================================\n";
                auto recentSongs = recentTracker.getRecentlyAdded();
                
                if (recentSongs.empty()) {
                    cout << "📭 No songs have been added recently." << endl;
                } else {
                    for (size_t i = 0; i < recentSongs.size(); ++i) {
                        cout << (i+1) << ". " << recentSongs[i]->title << " by " << recentSongs[i]->artist 
                             << " (" << recentSongs[i]->genre << ") - " << recentSongs[i]->duration << "s" << endl;
                    }
                    
                    // Show additional options
                    cout << "\n💡 Quick Actions:\n";
                    cout << "• Most recent: " << recentTracker.getLastAdded()->title << endl;
                    
                    // Show genre breakdown
                    unordered_map<string, int> genreCount;
                    for (auto* song : recentSongs) {
                        genreCount[song->genre]++;
                    }
                    
                    cout << "• Genre breakdown: ";
                    for (auto& pair : genreCount) {
                        cout << pair.first << "(" << pair.second << ") ";
                    }
                    cout << endl;
                }
                cout << "==========================================\n";
                break;
            }
            
            case 20: {
                // Clear Recently Added History
                recentTracker.clearRecentlyAdded();
                autoSave();
                break;
            }
            
            case 0: {
                cout << "👋 Thank you for using PlayWise! Saving your data..." << endl;
                break;
            }
            
            default: {
                cout << "❌ Invalid choice. Please try again." << endl;
                break;
            }
        }
        
    } while (choice != 0);

    // Save all data before exit
    autoSave();
    cout << "💾 Data saved successfully. Goodbye!" << endl;
    
    return 0;
}

/**
 * ============================================================================
 * OVERALL SYSTEM TIME COMPLEXITY ANALYSIS
 * ============================================================================
 * 
 * BEST CASE OPERATIONS: O(1)
 * - Song lookup by title (hash-based)
 * - Add song to playlist (tail insertion)
 * - Play count updates
 * - Skip tracking (bounded deque operations)
 * - Basic playback controls
 * - Undo operations
 * - Rating insertions
 * 
 * AVERAGE CASE OPERATIONS: O(n)
 * - Playlist traversal operations
 * - Song deletion by index
 * - Song movement within playlist
 * - Auto-replay song filtering
 * - Data persistence operations
 * - Playlist navigation
 * 
 * WORST CASE OPERATIONS: O(n log n)
 * - Song sorting by title/duration
 * - Auto-replay top songs selection (sorting calming songs)
 * - System snapshot generation with sorting
 * 
 * SPACE COMPLEXITY: O(n)
 * - Linear space for playlist storage
 * - Hash tables for lookups and play counts
 * - Bounded space for skip tracking (max 10 songs)
 * - Stack for playback history
 * - Map for rating system
 * 
 * ALGORITHMIC DESIGN CHOICES:
 * 
 * 1. DOUBLY-LINKED LIST for Playlist:
 *    - Pros: O(1) insertion/deletion at ends, bidirectional navigation
 *    - Cons: O(n) random access, higher memory overhead
 *    - Justification: Optimal for sequential playback and playlist manipulation
 * 
 * 2. HASH MAP for Song Lookup:
 *    - Pros: O(1) average search time
 *    - Cons: O(n) worst case (rare), memory overhead
 *    - Justification: Critical for fast song retrieval by title
 * 
 * 3. BALANCED BST (std::map) for Ratings:
 *    - Pros: O(log k) operations, sorted order
 *    - Cons: More complex than hash map
 *    - Justification: Enables efficient rating-based queries
 * 
 * 4. STACK for Playback History:
 *    - Pros: O(1) push/pop, natural undo semantics
 *    - Cons: No random access
 *    - Justification: Perfect for LIFO undo operations
 * 
 * 5. DEQUE for Skip Tracking:
 *    - Pros: O(1) insertion/deletion at both ends
 *    - Cons: Slightly more overhead than vector
 *    - Justification: Efficient circular buffer implementation
 * 
 * PERFORMANCE CHARACTERISTICS:
 * - Small playlists (< 100 songs): All operations feel instantaneous
 * - Medium playlists (100-1000 songs): Sorting operations may have slight delay
 * - Large playlists (> 1000 songs): Sorting and traversal operations noticeable
 * 
 * MEMORY USAGE:
 * - Per song: ~100-200 bytes (strings + pointers + metadata)
 * - Skip tracker: Fixed 10 song pointers = ~80 bytes
 * - Hash tables: ~2x song count for good performance
 * - Total: Approximately 500-800 bytes per song + fixed overhead
 * 
 * OVERALL SYSTEM COMPLEXITY:
 * - Time: O(n log n) for worst-case operations, O(n) for most operations
 * - Space: O(n) where n is the number of songs in the system
 * 
 * The system is optimized for fast lookups and efficient playlist management
 * while maintaining comprehensive functionality for music playback and
 * intelligent auto-replay features. The choice of data structures balances
 * performance, memory usage, and feature requirements effectively.
 * ============================================================================
 */
