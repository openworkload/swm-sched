#!/usr/bin/env escript
%% -*- coding: utf-8 -*-
%% -*- erlang -*-
%%! -smp enable -sname json-to-bin

-include("../deps/swm-core/include/wm_scheduler.hrl").

-define(SWM_LIB, "/../deps/swm-core/_build/default/lib/swm/ebin").
-define(SWM_JSX, "/../deps/swm-core/_build/default/lib/jsx/ebin").


logd(Format, Data) ->
  io:format(standard_error, Format, Data).

get_list_attr([], _, Entities) ->
  Entities;
get_list_attr([{_, AttrList}|T], {record, Name}, Entities) ->
  EmptyEntity = wm_entity:new(Name),
  Entity = json_to_entity(AttrList, EmptyEntity),
  get_list_attr(T, {record, Name}, [Entity|Entities]).

get_attr_value(List, {list, Type}) when is_list(List) ->
  get_list_attr(List, Type, []);
get_attr_value(Integer, integer) when is_integer(Integer)  ->
  Integer;
get_attr_value(Bin, atom) ->
  erlang:binary_to_atom(Bin, utf8);
get_attr_value(Bin, string) ->
  erlang:binary_to_list(Bin);
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
  logd("JSON RH: ~p~n", List),
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
  Decoded = jsx:decode(JsonBin),
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
  true = code:add_pathz(filename:dirname(escript:script_name()) ++ ?SWM_JSX),
  FinalBin = get_final_binary(JsonBin),
  io:put_chars(standard_io, binary_to_list(FinalBin)).

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
