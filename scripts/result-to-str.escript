#!/usr/bin/env escript
%% -*- coding: utf-8 -*-
%% -*- erlang -*-
%%! -smp enable -sname result-to-str

-define(SWM_LIB, "/../../swm/_build/default/lib/swm/ebin").
-define(SWM_JSX, "/../../swm/_build/default/lib/jsx/ebin").

print_timetable([]) ->
  ok;
print_timetable([TimetableItem|T]) ->
  StartTime = wm_entity:get_attr(start_time, TimetableItem),
  JobId     = wm_entity:get_attr(job_id, TimetableItem),
  JobNodes  = wm_entity:get_attr(job_nodes, TimetableItem),
  io:format("~p ~p ~p~n", [JobId, JobNodes, StartTime]),
  print_timetable(T).

print_metrics([]) ->
  ok;
print_metrics([Metric|T]) ->
  Name = wm_entity:get_attr(name, Metric),
  Integer = wm_entity:get_attr(value_integer, Metric),
  Float = wm_entity:get_attr(value_float64, Metric),
  io:format("~p ~p ~p~n", [Name, Integer, Float]),
  print_metrics(T).

print_meta(SchedulerResult) ->
  ReqId = wm_entity:get_attr(request_id, SchedulerResult),
  Status = wm_entity:get_attr(status, SchedulerResult),
  AstroTime = wm_entity:get_attr(astro_time, SchedulerResult),
  IdleTime = wm_entity:get_attr(idle_time, SchedulerResult),
  WorkTime = wm_entity:get_attr(work_time, SchedulerResult),
  io:format("request_id=~p~n", [ReqId]),
  io:format("status=~p~n", [Status]),
  io:format("astro_time=~p~n", [AstroTime]),
  io:format("idle_time=~p~n", [IdleTime]),
  io:format("work_time=~p~n", [WorkTime]).

print_result(SchedulerResult, []) ->
  Timetable = wm_entity:get_attr(timetable, SchedulerResult),
  Metrics = wm_entity:get_attr(metrics, SchedulerResult),
  print_meta(SchedulerResult),
  print_timetable(Timetable),
  print_metrics(Metrics);
print_result(SchedulerResult, ["astro-time"]) ->
  io:format("~p", [wm_entity:get_attr(astro_time, SchedulerResult)]);
print_result(SchedulerResult, ["idle-time"]) ->
  io:format("~p", [wm_entity:get_attr(idle_time, SchedulerResult)]);
print_result(SchedulerResult, ["work-time"]) ->
  io:format("~p", [wm_entity:get_attr(work_time, SchedulerResult)]).

main(Args) ->
  true = code:add_pathz(filename:dirname(escript:script_name()) ++ ?SWM_LIB),
  true = code:add_pathz(filename:dirname(escript:script_name()) ++ ?SWM_JSX),
  case wm_utils:read_stdin() of
    {ok, <<>>} ->
      print_result(wm_entity:new(<<"scheduler_result">>), Args);
    {ok, Bin} ->
      SchedulerResult = binary_to_term(Bin),
      print_result(SchedulerResult, Args);
    {error, Error} ->
      ip:format("Error reading stdin: ~p", [Error])
  end;
main(_) ->
  io:format("~nUsage: ~p ~n~n", [escript:script_name()]).
