#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libyang/libyang.h>

#define error_msg(msg, ...) fprintf(stderr, "%s:%u: " msg "\n", __FILE__, __LINE__, __VA_ARGS__)

#define ERR_INPUT_FAILURE -1
#define ERR_EXPECTATIONS_VIOLATED -2

struct ly_ctx* load_parser();
int read_model(struct ly_ctx* ctx, const char* yang_file);

struct lyd_node* read_data_file(struct ly_ctx* ctx, const char* json_file);

struct player_info {
  const char* team_name;
  const char* player_name;
  const char* season;
  int number;
  int scores;
};
int extract_player_info(const struct lyd_node* node, struct player_info* player);

int examine_data(const struct lyd_node* root);

int main(int nargs, const char** args) {
  (void)nargs;
  (void)args;

  struct ly_ctx* ctx = load_parser();
  if (!ctx) {
    return -1;
  }

  const char* data_path = "example-data.json";
  struct lyd_node* in_data = read_data_file(ctx, data_path);
  if (!in_data) {
    ly_ctx_destroy(ctx, 0);
    return -2;
  }

  printf("read data file %s successfully\n", data_path);

  int rc = examine_data(in_data);
  if (rc != 0) {
    ly_ctx_destroy(ctx, 0);
    return -4;
  }

  ly_ctx_destroy(ctx, 0);
  return 0;
}

struct lyd_node* read_data_file(struct ly_ctx* ctx, const char* json_file) {
  LYD_FORMAT data_fmt = LYD_JSON;
  int options = LYD_OPT_CONFIG;
  struct lyd_node* in_data = lyd_parse_path(ctx, json_file, data_fmt, options);
  if (!in_data) {
    error_msg("failed lyd_parse_path, json_file=%s", json_file);
    return 0;
  }

  return in_data;
}

struct ly_ctx* load_parser() {
  const char* search_dir = ".";
  int options = LY_CTX_TRUSTED;
  struct ly_ctx* ctx = ly_ctx_new(search_dir, options);
  if (!ctx) {
    error_msg("failed ly_ctx_new call (search_dir=%s, options=0x%x)",
              search_dir ? search_dir : "null", options);
    return 0;
  }

  if (0 != read_model(ctx, "example-sports.yang")) {
    ly_ctx_destroy(ctx, 0);
    return 0;
  }

  return ctx;
}

int read_model(struct ly_ctx* ctx, const char* yang_file) {
  LYS_INFORMAT lys_fmt = LYS_IN_YANG;
  const struct lys_module *mod = lys_parse_path(ctx, yang_file, lys_fmt);
  if (!mod) {
    const char* fmt_desc;
    switch (lys_fmt) {
      case LYS_IN_YANG:
        fmt_desc = "yang";
        break;
      case LYS_IN_YIN:
        fmt_desc = "yin";
        break;
      default:
        fmt_desc = "unknown";
    }
    error_msg("failed lys_parse_mem, lys_fmt=%d(%s), yang_file=\n%s",
              (int)lys_fmt, fmt_desc, yang_file);
    return ERR_INPUT_FAILURE;
  }

  return 0;
}

int examine_data(const struct lyd_node* root) {
  const char* name_path = "person/name";
  const char* team_played_path = "team/player[name='%s']";
  struct ly_set* names = lyd_find_path(root, name_path);
  if (!names) {
    error_msg("lyd_find_path(%s) returned null", name_path);
    return ERR_EXPECTATIONS_VIOLATED;
  }

  printf("found %d names with %s\n", names->number, name_path);

  char path_buf[256];

  int i;
  for (i = 0; i < names->number; i++) {
    const struct lyd_node_leaf_list* name_leaf =
        (const struct lyd_node_leaf_list*)names->set.g[i];

    if (name_leaf->schema->nodetype != LYS_LEAF) {
      error_msg("unexpected nodetype %d, index %d in path %s",
          name_leaf->schema->nodetype, i, name_path);
      continue;
    }
    const char* name = name_leaf->value_str;

    int len = snprintf(path_buf, sizeof(path_buf), team_played_path, name);
    if (len >= sizeof(path_buf)) {
      error_msg("internal error: player %d (%s) name too long for buffer, skipping",
          i, name);
      continue;
    }

    struct ly_set* played_set = lyd_find_path(root, path_buf);
    if (!played_set) {
      error_msg("lyd_find_path(%s) returned null", path_buf);
      continue;
    }
    printf("player %d: %s played for %d:\n", i, name, played_set->number);

    int j;
    for (j = 0; j < played_set->number; j++) {
      const struct lyd_node* player_node = played_set->set.d[j];
      struct player_info info;
      int rc = extract_player_info(player_node, &info);
      if (rc != 0) {
        error_msg("error extracting player info for idx %d under %s", j, path_buf);
        continue;
      }
      printf("  %s (scored %d as #%d in %s)\n", info.team_name, info.scores, info.number,
          info.season);
    }
    ly_set_free(played_set);
  }

#if 0
  char* xml_out = 0;
  LYD_FORMAT out_fmt = LYD_XML;
  int out_options = LYP_FORMAT | LYP_WD_TRIM;
  int rc = lyd_print_mem(&xml_out, root, out_fmt, out_options);

  if (rc) {
    error_msg("failed lyd_print_mem: %d", rc);
    if (xml_out) {
      free(xml_out);
    }
    return;
  }

  printf("dumping input as xml:\n%s", xml_out);
  free(xml_out);
#endif

  return 0;
}

int extract_player_info(const struct lyd_node* node, struct player_info* player) {
  if (node->schema->nodetype != LYS_LIST) {
    error_msg("unexpected nodetype %d", node->schema->nodetype);
    return ERR_EXPECTATIONS_VIOLATED;
  }
  if (0 != strcmp(node->schema->name, "player")) {
    error_msg("unexpected node schema name %s", node->schema->name);
    return ERR_EXPECTATIONS_VIOLATED;
  }
  struct ly_set* team_name_set = lyd_find_path(node->parent, "name");
  if (!team_name_set) {
    error_msg("lyd_find_path(%s) returned null", "name");
    return ERR_EXPECTATIONS_VIOLATED;
  }
  if (team_name_set->number != 1) {
    error_msg("unexpected number of team names (%d)", team_name_set->number);
    if (team_name_set->number < 1) {
      return ERR_EXPECTATIONS_VIOLATED;
    }
  }
  memset(player, 0, sizeof(*player));
  const struct lyd_node_leaf_list* team_name_leaf =
      (struct lyd_node_leaf_list*)team_name_set->set.d[0];
  player->team_name = team_name_leaf->value_str;
  ly_set_free(team_name_set);

  struct lyd_node* field_node;
  LY_TREE_FOR(node->child, field_node) {
    if (field_node->schema->nodetype != LYS_LEAF) {
      error_msg("unexpected nodetype %d (%s)",
          field_node->schema->nodetype, field_node->schema->name);
      return ERR_EXPECTATIONS_VIOLATED;
    }
    struct lys_node_leaf* schema = (struct lys_node_leaf*)field_node->schema;
    struct lyd_node_leaf_list* field = (struct lyd_node_leaf_list*)field_node;
    if (0 == strcmp("name", schema->name)) {
      player->player_name = field->value_str;
    } else if (0 == strcmp("season", schema->name)) {
      player->season = field->value_str;
    } else if (0 == strcmp("number", schema->name)) {
      if (schema->type.base != LY_TYPE_UINT16) {
        error_msg("unexpected field value type for number: %d", schema->type.base);
        return ERR_EXPECTATIONS_VIOLATED;
      }
      player->number = field->value.uint16;
    } else if (0 == strcmp("scores", schema->name)) {
      if (schema->type.base != LY_TYPE_UINT16) {
        error_msg("unexpected field value type for scores: %d", schema->type.base);
        return ERR_EXPECTATIONS_VIOLATED;
      }
      player->scores = field->value.uint16;
    }
  }

  return 0;
}
