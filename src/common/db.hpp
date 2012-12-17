// WARNING: there is a system header by this name
#ifndef DB_HPP
#define DB_HPP
# include "sanity.hpp"

# include <functional>

/// Number of tree roots
// Somewhat arbitrary - larger wastes more space but is faster for large trees
// num % HASH_SIZE minimize collisions even for similar num
# define HASH_SIZE (256+27)

typedef enum dbn_color
{
    RED,
    BLACK,
} dbn_color;

typedef intptr_t numdb_key_t;
typedef union db_key_t
{
    char *ms __attribute__((deprecated));
    const char* s;
    numdb_key_t i;

    db_key_t(numdb_key_t n) : i(n) {}
    db_key_t(const char * z) : s(z) {}
} db_key_t;
typedef void* db_val_t;
typedef uint32_t hash_t;
typedef std::function<void(db_key_t, db_val_t)> db_func_t;

/// DataBase Node
struct dbn
{
    struct dbn *parent, *left, *right;
    dbn_color color;
    db_key_t key;
    db_val_t data;
};

typedef enum dbt_type
{
    DB_NUMBER,
    DB_STRING,
} dbt_type;

/// DataBase Table
struct dbt
{
    dbt_type type;
    /// Note, before replacement, key/values to be replaced
    // TODO refactor to decrease/eliminate the uses of this?
    void(*release)(db_key_t, db_val_t) __attribute__((deprecated));
    /// Maximum length of a string key - TODO refactor to ensure all strings are NUL-terminated
    size_t maxlen __attribute__((deprecated));
    /// The root trees
    struct dbn *ht[HASH_SIZE];
};

# define strdb_search(t,k)   db_search((t), (db_key_t)(k))
# define strdb_insert(t,k,d) db_insert((t), (db_key_t)(k), (db_val_t)(d))
# define strdb_erase(t,k)    db_erase((t), (db_key_t)(k))
# define strdb_foreach       db_foreach
# define strdb_final         db_final
# define numdb_search(t,k)   db_search((t), (db_key_t)(k))
# define numdb_insert(t,k,d) db_insert((t), (db_key_t)(k), (db_val_t)(d))
# define numdb_erase(t,k)    db_erase((t), (db_key_t)(k))
# define numdb_foreach       db_foreach
# define numdb_final         db_final

/// Create a map from char* to void*, with strings possibly not null-terminated
struct dbt *strdb_init(size_t maxlen);
/// Create a map from int to void*
struct dbt *numdb_init(void);
/// Return the value corresponding to the key, or NULL if not found
db_val_t db_search(struct dbt *table, db_key_t key);
/// Add or replace table[key] = data
// if it was already there, call release
struct dbn *db_insert(struct dbt *table, db_key_t key, db_val_t data);
/// Remove a key from the table, returning the data
db_val_t db_erase(struct dbt *table, db_key_t key);

/// Execute a function for every element, in unspecified order
void db_foreach(struct dbt *, db_func_t);
// opposite of init? Calls release for every element and frees memory
// This probably isn't really needed: we don't have to free memory while exiting
void db_final(struct dbt *, db_func_t) __attribute__((deprecated));

#endif // DB_HPP
