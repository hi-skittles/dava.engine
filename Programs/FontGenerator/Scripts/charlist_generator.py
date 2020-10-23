from optparse import OptionParser
import unicodedata

default_charsets = ['latin_1', 'cp1251']

def get_characters(charsets):
    characters = []
    for charset in charsets:
        for i in xrange(1, 256):
            try:
                uChar = chr(i).decode(charset)
                group = unicodedata.category(uChar)
                # Ignore control characters and duplicates
                if group != 'Cc' and not uChar in characters:
                    characters.append(uChar)
            except:
                # Ignore encoding/decoding errors
                pass
    return characters


def write_to_file(filename, characters):
    with open(filename, "w") as f:
        for c in characters:
            f.write(c.encode('utf-8'))


def main():
    parser = OptionParser(usage="Usage: %prog [options] charset1 [charset2...]")
    parser.add_option("-o", "--output", dest="filename", default=None,
                    help="write charset's characters to output file")
    (options, charsets) = parser.parse_args()

    if len(charsets) == 0:
        charsets = default_charsets

    characters = get_characters(charsets)

    if options.filename:
        write_to_file(options.filename, characters)
    else:
        print "".join(characters).encode('utf-8')

if __name__ == "__main__":
    main()