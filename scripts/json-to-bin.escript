#!/usr/bin/env escript
%% -*- coding: utf-8 -*-
%% -*- erlang -*-
%%! -smp enable -sname json-to-bin
%%
%% Convert scheduler test JSON to the ErlBIN stream expected by swm-sched.
%% Uses OTP stdlib json (swm-core no longer ships jsx).
%%
%% Resource lists must be JSON arrays of objects, for example:
%%   "request": [ {"name": "node", "count": 1}, {"name": "mem", "count": 8} ]
%% Duplicate object keys are not preserved by json:decode/1.

-include("../deps/swm-core/include/wm_scheduler.hrl").

-define(SWM_LIB, "/../deps/swm-core/_build/default/lib/swm/ebin").


logd(Format, Data) ->
  io:format(standard_error, Format, Data).

%% json:decode/1 returns maps; older jsx-based helpers expect proplists.
maps_to_proplists(Map) when is_map(Map) ->
  [{K, maps_to_proplists(V)} || {K, V} <- maps:to_list(Map)];
maps_to_proplists(List) when is_list(List) ->
  [maps_to_proplists(X) || X <- List];
maps_to_proplists(Other) ->
  Other.

get_list_attr([], _, Entities) ->
  Entities;
%% Array of entity objects (preferred; works with stdlib json).
get_list_attr([AttrList|T], {record, Name}, Entities) when is_list(AttrList) ->
  EmptyEntity = wm_entity:new(Name),
  Entity = json_to_entity(AttrList, EmptyEntity),
  get_list_attr(T, {record, Name}, [Entity|Entities]);
%% Legacy jsx shape: object with repeated type keys, e.g. [{"resource", Attrs}, ...].
get_list_attr([{_, AttrList}|T], {record, Name}, Entities) when is_list(AttrList) ->
  EmptyEntity = wm_entity:new(Name),
  Entity = json_to_entity(AttrList, EmptyEntity),
  get_list_attr(T, {record, Name}, [Entity|Entities]).

get_attr_value(List, {list, Type}) when is_list(List) ->
  get_list_attr(List, Type, []);
get_attr_value(Integer, integer) when is_integer(Integer)  ->
  Integer;
get_attr_value(Bin, atom) when is_binary(Bin) ->
  erlang:binary_to_atom(Bin, utf8);
get_attr_value(Atom, atom) when is_atom(Atom) ->
  Atom;
get_attr_value(Bin, string) when is_binary(Bin) ->
  erlang:binary_to_list(Bin);
get_attr_value(List, string) when is_list(List) ->
  List;
get_attr_value(Bin, Other) ->
  logd("ERROR: unknown attribute type: ~p (~p)~n", [Bin, Other]).

json_to_entities([], _, Entities) ->
  Entities;
json_to_entities([List|T], EntityNameBin, Entities) when is_list(List) ->
  Entity = wm_entity:new(EntityNameBin),
  json_to_entities(T, EntityNameBin, [json_to_entity(List, Entity)|Entities]).

json_to_entity([], Entity) ->
  Entity;
json_to_entity([{AttrBin,ValueBin}|T], OldEntity) ->
  Attr = binary_to_atom(AttrBin, utf8),
  Name = element(1, OldEntity),
  Type = wm_entity:get_type(Name, Attr),
  Value = get_attr_value(ValueBin, Type),
  NewEntity = wm_entity:set({Attr, Value}, OldEntity),
  json_to_entity(T, NewEntity).

json_to_rh([], RhMap) ->
  RhMap;
json_to_rh([[{ItemBin,Id}]|T], RhMap) ->
  NewRhMap = maps:put({binary_to_atom(ItemBin, utf8), Id}, [], RhMap),
  json_to_rh(T, NewRhMap);
json_to_rh([[{ItemBin,Id},{<<"sub">>,Sub}]|T], RhMap) ->
  SubMap = json_to_rh(Sub, maps:new()),
  NewRhMap = maps:put({binary_to_atom(ItemBin, utf8), Id}, SubMap, RhMap),
  json_to_rh(T, NewRhMap);
json_to_rh([[{<<"sub">>,Sub},{ItemBin,Id}]|T], RhMap) ->
  SubMap = json_to_rh(Sub, maps:new()),
  NewRhMap = maps:put({binary_to_atom(ItemBin, utf8), Id}, SubMap, RhMap),
  json_to_rh(T, NewRhMap).

json_to_map([], FinalMap) ->
  FinalMap;
json_to_map([{<<"rh">>,List}|T], OldMap) ->
  logd("JSON RH: ~p~n", [List]),
  RhMap = json_to_rh(List, maps:new()),
  logd("RH=~p~n", [RhMap]),
  NewMap = maps:put(rh, RhMap, OldMap),
  json_to_map(T, NewMap);
json_to_map([{EntityNameBin,List}|T], OldMap) ->
  Entities = json_to_entities(List, EntityNameBin, []),
  logd("ENTITIES=~p (~p)~n", [Entities, EntityNameBin]),
  Name = binary_to_atom(EntityNameBin, utf8),
  OldList = maps:get(Name, OldMap, []),
  NewMap = maps:put(Name, Entities ++ OldList, OldMap),
  json_to_map(T, NewMap);
json_to_map(Other, _) ->
  io:format("Could not convert json to map: ~p~n", [Other]),
  #{}.

get_final_binary(JsonBin) ->
  Decoded = maps_to_proplists(json:decode(JsonBin)),
  Map = json_to_map(Decoded, maps:new()),
  SchedBin = erlang:term_to_binary(maps:get(scheduler, Map, <<>>)),
  RhBin    = erlang:term_to_binary(wm_utils:map_to_list(maps:get(rh, Map, <<>>))),
  JobsBin  = erlang:term_to_binary(maps:get(job, Map, <<>>)),
  GridBin  = erlang:term_to_binary(maps:get(grid, Map, <<>>)),
  ClustBin = erlang:term_to_binary(maps:get(cluster, Map, <<>>)),
  PartsBin = erlang:term_to_binary(maps:get(partition, Map, <<>>)),
  NodesBin = erlang:term_to_binary(maps:get(node, Map, <<>>)),

  logd("~nFINAL MAP=~p~n~n", [Map]),
  Bin0 = wm_sched_utils:add_input(?TOTAL_DATA_TYPES, <<>>, <<>>),
  logd("~nJOBS BIN=~p~n~n", [wm_sched_utils:add_input(?DATA_TYPE_JOBS, JobsBin, <<>>)]),
  Bin1 = wm_sched_utils:add_input(?DATA_TYPE_SCHEDULERS, SchedBin, Bin0),
  Bin2 = wm_sched_utils:add_input(?DATA_TYPE_RH, RhBin, Bin1),
  Bin3 = wm_sched_utils:add_input(?DATA_TYPE_JOBS, JobsBin, Bin2),
  Bin4 = wm_sched_utils:add_input(?DATA_TYPE_GRID, GridBin, Bin3),
  Bin5 = wm_sched_utils:add_input(?DATA_TYPE_CLUSTERS, ClustBin, Bin4),
  Bin6 = wm_sched_utils:add_input(?DATA_TYPE_PARTITIONS, PartsBin, Bin5),
  wm_sched_utils:add_input(?DATA_TYPE_NODES, NodesBin, Bin6).

convert_and_print(JsonBin) ->
  FinalBin = get_final_binary(JsonBin),
  % CI/act often uses a UTF-8 locale where standard_io encoding is unicode;
  % writing raw scheduler bytes through unicode would UTF-8-escape values like 0x83.
  ok = io:setopts(standard_io, [{encoding, latin1}]),
  ok = file:write(standard_io, FinalBin).

main([Filename]) ->
  true = code:add_pathz(filename:dirname(escript:script_name()) ++ ?SWM_LIB),
  case wm_utils:read_file(Filename, [binary]) of
    {ok, JsonBin} ->
      convert_and_print(JsonBin);
    {error, Reason} ->
      io:format("Could not read '~p': ~p~n", [Filename, Reason])
  end;
main([]) ->
  true = code:add_pathz(filename:dirname(escript:script_name()) ++ ?SWM_LIB),
  case wm_utils:read_stdin() of
    {ok, JsonBin} ->
      convert_and_print(JsonBin);
    {error, Error} ->
      io:format("Could not read data from STDIN: ~p~n", [Error])
  end;
main(_) ->
  logd("~nUsage: cat BIN | ~p~n~n", [escript:script_name()]).
