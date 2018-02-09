-- name: number by numeral word
SELECT number FROM numerals WHERE word = :word::text

-- name: numeral word by number
SELECT word FROM numerals WHERE number >= :number::bigint
