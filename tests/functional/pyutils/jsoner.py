#!/usr/bin/env python3

import copy
import json
import logging
import os

from subprocess import Popen, PIPE, STDOUT


log = logging.getLogger("scheduler")


class JsonManipulator:

    def __init__(self):
        self._my_dir = os.path.dirname(os.path.realpath(__file__))

    def generate(self, opts):
        maps = []
        for file_path in opts.get("SETUP_FILES", []):
            with open(file_path) as data_file:
                new_data = data_file.read()
                decoder = json.JSONDecoder(object_pairs_hook=self._parse_object_pairs)
                obj = decoder.decode(new_data)
                maps.append(obj)
        (final_map, for_rh_map) = self._apply_test_statements(maps, opts)
        self._add_new_objs_to_rh(final_map, for_rh_map)
        return final_map

    def convert_to_bin(self, json_map):
        json_str = json.dumps(json_map)
        json_bytes = json_str.encode(encoding='utf_8')
        log.debug("Convert json: %s" % json_str)
        dir = os.path.join(self._my_dir, "..", "..", "..", "swm-sched", "scripts")
        cmd = "json-to-bin.escript"
        convertor = os.path.join(dir, cmd)
        cwd = os.path.join(self._my_dir, "..")
        p = Popen(convertor, cwd=cwd, shell=True, stdout=PIPE, stderr=PIPE, stdin=PIPE)
        out = p.communicate(input=json_bytes)[0]
        log.debug("%s: %s" % (cmd, out))
        return out


    def _make_unique(self, key, dct):
        counter = 0
        unique_key = key
        while unique_key in dct:
            counter += 1
            unique_key = '{}_{}'.format(key, counter)
        return unique_key

    def _parse_object_pairs(self, pairs):
        dct = {}
        for key, value in pairs:
            if key in dct:
                key = self._make_unique(key, dct)
            dct[key] = value
        return dct

    def _search_and_add_to_rh(self, item_name, id_map, rh_list):
        for x in rh_list:
            found = None
            if isinstance(x, dict):
                for key,value in x.items():
                    if key == "sub":
                        self._search_and_add_to_rh(item_name, id_map, value)
                    elif key == item_name and value in id_map.keys():
                        found = id_map[value]
                        break
            if found:
                for new_id in found:
                    rh_list.append({item_name: new_id})
                rh_list.remove(x)

    def _add_new_objs_to_rh(self, full_map, for_rh):
        rh = full_map.get("rh")
        for item_name,id_map in for_rh.items():
            item = self._search_and_add_to_rh(item_name, id_map, rh)

    def _parse(self, statement, add_objs_func, obj_name, map):
        n = statement.find("=")
        pp = statement[n+1:].split(":")
        if len(pp) == 2:
            job_id = pp[0]
            amount = int(pp[1])
            return add_objs_func(obj_name, job_id, amount, map)
        return {}

    def _set_obj_id(self, name, obj, id, index):
        if name == "job":
            obj["id"] = id + "_" + str(index)
        elif name == "node":
            obj["id"] = index

    def _add_obj(self, name, id, amount, map):
        log.debug("Add %d %ss like %s with id=%s\n" % (amount, name, name, id))
        new_map = {name: map[name]}
        for_rh = {}
        for obj_tuple in map[name]:
            if obj_tuple["id"] != id:
                continue
            for i in range(amount):
                new_obj = copy.copy(obj_tuple)
                self._set_obj_id(name, new_obj, id, i)
                new_map[name].append(new_obj)
                for_rh.setdefault(id, []).append(new_obj["id"])
            map[name].remove(obj_tuple)
        return ({**map, **new_map}, for_rh)

    def _apply_test_statements(self, maps, opts):
        final_map = {}
        for_rh_map = {}
        for statement in opts.get("TEST_STATEMENTS", [""]):
            for map in maps:
                if "job" in statement and "job" in map:
                    (new_map, new_for_rh) = self._parse(statement, self._add_obj, "job", map)
                    if new_map:
                        final_map = {**final_map, **new_map}
                elif "node" in statement and "node" in map:
                    (new_map, new_for_rh) = self._parse(statement, self._add_obj, "node", map)
                    if new_map:
                        final_map = {**final_map, **new_map}
                        for_rh_map["node"] = {**for_rh_map.setdefault("node", {}), **new_for_rh}
                else:
                    final_map = {**final_map, **map}
        return final_map, for_rh_map
