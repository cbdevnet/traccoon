char* QUERY_TORRENT_COUNT = "SELECT count(torrents.id) AS torrents FROM torrents;";
char* QUERY_TORRENT_ID = "SELECT id FROM torrents WHERE hash=?;";
char* QUERY_TORRENT_PEERS = "SELECT peer_id, ip, port, protover FROM peers WHERE torrent=? ORDER BY RANDOM() LIMIT 50;";
char* ADD_TORRENT_PEER = "INSERT INTO peers (torrent, peer_id, ip, port, expires, protover) VALUES (?,?,?,?,?,?);";
char* QUERY_PEER_BY_ID = "SELECT id FROM peers WHERE torrent=? AND peer_id=?;";
char* UPDATE_PEER_DATA = "UPDATE peers SET left=?, status=?, expires=? WHERE id=?;";
char* QUERY_PRUNE_PEERS = "DELETE FROM peers WHERE expires<?;";
char* INCREASE_COMPLETION_COUNT = "UPDATE torrents SET downloaded=downloaded+1 WHERE id=?;";
char* GET_COMPLETION_COUNT = "SELECT downloaded FROM torrents WHERE id=?;";
char* GET_SEED_COUNT = "SELECT count(peers.id) AS seeders FROM peers WHERE torrent=? AND left=0;";
char* GET_PEERS_COUNT = "SELECT count(peers.id) AS seeders FROM peers WHERE torrent=? AND left!=0;";

/*
Used/required database fields
	torrents
		id
		hash
		downloaded
	peers
		peer_id
		ip
		port
		protover
		torrent
		expires
		id
		left
		status
*/

char* CREATE_TORRENT_TABLE = "CREATE TABLE torrents (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE, hash TEXT NOT NULL UNIQUE, downloaded INTEGER DEFAULT (0));";
char* CREATE_PEERS_TABLE = "CREATE TABLE peers (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE, torrent INTEGER NOT NULL REFERENCES torrents (id) ON DELETE CASCADE, peer_id TEXT, ip TEXT NOT NULL, port INTEGER NOT NULL, protover INTEGER, [left] INTEGER DEFAULT (0), status INTEGER, expires INTEGER NOT NULL DEFAULT (0));";