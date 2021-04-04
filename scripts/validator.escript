#!/usr/bin/env escript
%% -*- coding: utf-8 -*-
%% -*- erlang -*-
%%! -smp enable -sname validator

-define(SWM_LIB, "/../../swm/_build/default/lib/swm/ebin").
-define(SWM_JSX, "/../../swm/_build/default/lib/jsx/ebin").

logd(Format, Data) ->
  io:format(standard_error, "[VALIDATOR] " ++ Format, Data).

validate(SchedulerResult, FileWithExpectedResult) ->
  ActualTimetable = wm_entity:get_attr(timetable, SchedulerResult),
  logd("actual timetable:   ~p~n", [ActualTimetable]),
  case file:consult(FileWithExpectedResult) of
    {ok, [ExpectedTimetable]} ->
      logd("expected timetable: ~p~n", [ExpectedTimetable]),
      case ExpectedTimetable == ActualTimetable of
        true ->
          halt(0);
        _ ->
          halt(1)
      end;
    _ ->
      logd("could not parse \"~s\"~n", [FileWithExpectedResult]),
      halt(1)
  end.

main(["-e", FileWithExpectedResult]) ->
  true = code:add_pathz(filename:dirname(escript:script_name()) ++ ?SWM_LIB),
  true = code:add_pathz(filename:dirname(escript:script_name()) ++ ?SWM_JSX),
  case wm_utils:read_stdin() of
    {ok, <<>>} ->
      validate(wm_entity:new(<<"scheduler_result">>), FileWithExpectedResult);
    {ok, Bin} ->
      SchedulerResult = binary_to_term(Bin),
      validate(SchedulerResult, FileWithExpectedResult);
    {error, Error} ->
      logd("error reading stdin: ~p", [Error])
  end;
main(_) ->
  io:format("Usage: cat FILE_RESULT | validator.escript -e FILE_EXPECTED~n").
