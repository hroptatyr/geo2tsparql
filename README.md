geo2t
=====

Embed bitemporal data in geospatial databases.

Geospatial features in databases seem ubiquitous.  Rightly so?  Maybe.
It's irrelevant in my domain.  What really is relevant, important and
probably even harder to get right on your own is temporality.

Everything in the world of finance needs temporality: What did I think
yesterday this dividend was, it's correct now but yesterday I based my
trading on the previous value, I should have taken a note.  It gets
trickier when the tracked value is a time itself or a tuple of times:
Think a timeseries of the weather forecast for the next 10 days along
with the actually observed weather data.

Anyway, most databases and triple stores don't have any means to cope
with temporal data *efficiently*, i.e. querying by validity time (what
futures contracts existed on 02 April 2016) or by system time (give me
yesterday's weather forecast for today) is not index-aided.

The `geo2t` tool simply maps temporal regions onto earth and returns
the geometry in geospatial coordinates, the converse is supported too
of course.

Example
-------

    $ geo2t <<EOF
    BOX(2000-02-29Z/2016-03-31Z)
    EOF
    BOX2D(0.46093750000000000 46.40324725115740989, 46.36718750000000000 90.00000000000000000)

and to map it back:

    $ geo2t <<EOF
    BOX2D(0.46093750000000000 46.40324725115740989, 46.36718750000000000 90.00000000000000000)
    EOF
    BOX2D(2000-02-29Z/2016-03-31Z, 2016-04-05T14:46:32.000Z+)

the second dimension (system time) is inserted automatically if omitted.
