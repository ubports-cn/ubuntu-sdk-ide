(:*******************************************************:)
(: Test: K-SeqExistsFunc-11                              :)
(: Written by: Frans Englich                             :)
(: Date: 2007-11-22T11:31:22+01:00                       :)
(: Purpose: The Dynamic Context property 'current dateTime' must have an explicit timezone when presented as a xs:date. :)
(:*******************************************************:)
exists(timezone-from-date(current-date()))