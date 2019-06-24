import numpy as np
import pandas as pd
import preprocessor as p
import gensim.utils
from gensim import corpora
from konlpy.tag import Mecab
from sklearn.feature_extraction.text import CountVectorizer

stopwords = ['아','휴','아이구','아이쿠','아이고','어','나','우리','저희','따라','의해','을','를','에','의','가','아니','와아','응','아이','참나','트윗','트위터','트친','멘션',
             'ㄷㄷㄷ','로','ㄷ','ㄷㄷ','다른','물론','또한','그리고','다','한다면','잠시','잠깐','만약','만일','무엇','무슨','어느','ㅠㅠ','ㅠ','그','너희','그들','너희들','타인','것','것들','너','나','우리',
             '등','등등','제','대박','단지','다만','약간','다소','좀','조금','다수','몇','얼마','지만','와','과','만','도','주','ㅋㅋㅋ','ㅋㅋ','ㅋ','ㅎㅎ','ㅎ',
             '고로','언제','야','이봐','어이','여러분','흥','휴','헉헉','저기','결국','저것','그때','그럼','그러면','그래','그때']

# mallet_path = '/home/mixify/mdsl/snsdatamining/mallet-2.0.8/bin/mallet'

mecab = Mecab()

def remove_stopwords(stopwords,tokenized_texts):
    result = []
    for idx,txt in enumerate(tokenized_texts):
        result.append([])
        for word in txt:
            if(word not in stopwords):
                result[idx].append(word)
    return result

def pre_processing_data(data_name):
    data = pd.read_json(data_name,lines=True) # read data

    texts = data['text'] # get only texts from data

    prepreocessed_texts = [p.clean(text) for text in texts] # pre_process text

    prepreocessed_texts_by_gensim = [' '.join(gensim.utils.simple_preprocess(text, deacc=True,min_len=2))
                                     for text in prepreocessed_texts] # remove special characters

    tokenized_texts = [mecab.nouns(text) for text in prepreocessed_texts_by_gensim] # tokenize nouns in texts

    result = remove_stopwords(stopwords,tokenized_texts) # remove stopwords

    return result


def save_topic_distribution(data_name,data):
    loc = data_name.rfind('/')

    save_name = data_name[loc+1:].replace('txt','model')

    id2word = corpora.Dictionary(data) # make dictionary and ids

    corpus = [id2word.doc2bow(text) for text in data] # make corpus

    lda = gensim.models.LdaMulticore(corpus=corpus, id2word=id2word, num_topics=10)

    lda.save(save_name)

def get_top_n_gram_words(corpus, n=None, gram=1):
    def dummy(doc):
        return doc
    vec1 = CountVectorizer(ngram_range=(gram,gram), preprocessor=dummy, tokenizer=dummy,
            max_features=2000).fit(corpus) # count vectorizier corpus with grams

    bag_of_words = vec1.transform(corpus) # make form to BOW

    sum_words = bag_of_words.sum(axis=0) # sum same keywords

    words_freq = [(word, sum_words[0, idx]) for word, idx in
                  vec1.vocabulary_.items()] # make words, frequency list

    words_freq =sorted(words_freq, key = lambda x: x[1],
                reverse=True) # sort the datas

    return words_freq[:n]

    # ladmallet = gensim.models.wrappers.LdaMallet(mallet_path,corpus=corpus,num_topics=10,id2word=id2word) # learn to make topic


     # compute index
    # corpora.MmCorpus.serialize(save_name, ladmallet[corpus])
    # idx.save(save_name)
