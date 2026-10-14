# SPDX-License-Identifier: MIT
#
# Classify the CRLF-normalized serial log of one test boot.
#
# The controller passes the expected case name, assertion count, build
# identity, and fault pattern with -v. The program prints one verdict word:
# "ok", or a failure reason. A record is accepted only at the start of a
# line. Any malformed, duplicate, or out-of-order record fails the case.
#
# The starter and the reference OS share this file byte for byte. The reference
# OS is instructor material, and a student does not have it.
#
# Reasons: hang, stale-build, malformed-record, duplicate-record,
#          out-of-order-record, wrong-case, wrong-assertion-count,
#          assertion-failed, unrelated-panic, multiple-panics,
#          expected-fault-missing, unexpected-completion, no-completion,
#          no-idle, ok.

BEGIN {
    bad = 0
    nbuild = 0; nbegin = 0; nassert = 0; nok = 0; nfail = 0; nend = 0; nidle = 0
    np = 0
}

function fail(r) { print r; bad = 1; exit }

# A panic prefix is "PANIC: " then a file field then ":" digits ":" space. The
# file field may hold any character, spaces and colons included, but not the
# text "PANIC: ". The kernel prints the panic without a leading newline, so
# console text can precede it on the same line. A bare "PANIC: " with no file
# and line fields is ordinary output.
#
# Count every prefix on the line: split the line on "PANIC: " and test each
# piece for ":<digits>: ". Two or more panic events, on one line or on separate
# lines, are a contradiction. The controller's settle scan matches the same
# rule (tests/run-tests.sh).
!/^KTEST / {
    n = split($0, parts, "PANIC: ")
    for (i = 2; i <= n; i++) {
        if (match(parts[i], /:[0-9]+: /)) {
            np++
            pline[np] = NR
            pmsg[np] = substr(parts[i], RSTART + RLENGTH)
        }
    }
    next
}

# A panic ends the transcript and spins forever, so no record can follow it.
np > 0 { fail("out-of-order-record") }

/^KTEST BUILD / {
    if ($0 !~ /^KTEST BUILD id=[0-9a-f]+$/) fail("malformed-record")
    if (nbuild > 0) fail("duplicate-record")
    if (nbegin > 0 || nassert > 0 || nend > 0 || nidle > 0) fail("out-of-order-record")
    nbuild = 1
    id = $0; sub(/^KTEST BUILD id=/, "", id)
    if (id != want_id) fail("stale-build")
    next
}

/^KTEST BEGIN / {
    if ($0 !~ /^KTEST BEGIN name=[A-Za-z0-9_]+$/) fail("malformed-record")
    if (nbuild == 0) fail("out-of-order-record")
    if (nbegin > 0) fail("duplicate-record")
    if (nassert > 0 || nend > 0) fail("out-of-order-record")
    nbegin = 1; beginline = NR
    nm = $0; sub(/^KTEST BEGIN name=/, "", nm)
    if (nm != want_case) fail("wrong-case")
    next
}

/^KTEST ASSERT / {
    if ($0 !~ /^KTEST ASSERT (ok|fail) at=[^ ]+ expr=/) fail("malformed-record")
    if (nbegin == 0) fail("out-of-order-record")
    if (nend > 0) fail("out-of-order-record")
    nassert++
    if ($0 ~ /^KTEST ASSERT ok /) {
        nok++
    } else {
        nfail++
    }
    next
}

/^KTEST END / {
    if ($0 !~ /^KTEST END name=[A-Za-z0-9_]+ assertions=[0-9]+ failures=[0-9]+$/) fail("malformed-record")
    if (nbegin == 0) fail("out-of-order-record")
    if (nend > 0) fail("duplicate-record")
    nend = 1; endline = NR
    nm = $0; sub(/^KTEST END name=/, "", nm); sub(/ .*/, "", nm)
    if (nm != want_case) fail("wrong-case")
    ea = $0; sub(/.* assertions=/, "", ea); sub(/ .*/, "", ea)
    ef = $0; sub(/.* failures=/, "", ef)
    end_asserts = ea + 0; end_failures = ef + 0
    next
}

/^KTEST IDLE$/ {
    if (nend == 0) fail("out-of-order-record")
    if (nidle > 0) fail("duplicate-record")
    nidle = 1
    next
}

# A line that starts a record but matches no record is malformed.
{ fail("malformed-record") }

END {
    if (bad) exit
    if (nbuild == 0 || nbegin == 0) { print "hang"; exit }

    if (fault != "" && fault != "-") {
        # A negative case. The declared fault must appear, alone, after BEGIN,
        # and no failed assertion may appear.
        if (nend > 0) { print "unexpected-completion"; exit }
        if (np == 0) { print "expected-fault-missing"; exit }
        if (np > 1) { print "multiple-panics"; exit }
        if (pline[1] < beginline) { print "unrelated-panic"; exit }
        if (nfail > 0) { print "assertion-failed"; exit }
        if (index(pmsg[1], fault) == 0) { print "unrelated-panic"; exit }
        if (nassert != want) { print "wrong-assertion-count"; exit }
        print "ok"; exit
    }

    # A positive case. The only accepted panic is the idle panic, after a
    # validated completion record, and it must be the only panic.
    if (np > 1) { print "multiple-panics"; exit }
    if (np == 1) {
        if (!(nend > 0 && pline[1] > endline && pmsg[1] == "switched to idle process"))
            { print "unrelated-panic"; exit }
    }
    if (nend == 0) { print "no-completion"; exit }
    if (end_asserts != want || nassert != want) { print "wrong-assertion-count"; exit }
    if (end_failures != nfail || nfail != 0) { print "assertion-failed"; exit }
    if (nidle > 0 || np == 1) { print "ok"; exit }
    print "no-idle"
}
